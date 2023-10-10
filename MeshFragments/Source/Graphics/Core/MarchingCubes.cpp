#include "stdafx.h"
#include "MarchingCubes.h"

#include "Graphics/Core/ShaderList.h"

// [Public methods]

MarchingCubes::MarchingCubes(RegularGrid& regularGrid, const uvec3& numDivs, unsigned maxTriangles)
{
    _numDivs = numDivs + uvec3(2);
    _maxTriangles = maxTriangles;
    _numThreads = _numDivs.x * _numDivs.y * _numDivs.z;
    _numGroups = ComputeShader::getNumGroups(_numThreads);
	unsigned maxNumPoints = _numThreads * _maxTriangles * 3;
	_indices = new unsigned[maxNumPoints];
	std::iota(_indices, _indices + maxNumPoints, 0);

	// Shaders
	_buildMarchingCubesShader = ShaderList::getInstance()->getComputeShader(RendEnum::BUILD_MARCHING_CUBES_FACES);
	_fuseSimilarVerticesShader = ShaderList::getInstance()->getComputeShader(RendEnum::FUSE_VERTICES);
	_marchingCubesShader = ShaderList::getInstance()->getComputeShader(RendEnum::MARCHING_CUBES);

	_computeMortonShader = ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_MORTON_CODES_FRACTURER);
	_bitMaskShader = ShaderList::getInstance()->getComputeShader(RendEnum::BIT_MASK_RADIX_SORT);
	_reduceShader = ShaderList::getInstance()->getComputeShader(RendEnum::REDUCE_PREFIX_SCAN);
	_downSweepShader = ShaderList::getInstance()->getComputeShader(RendEnum::DOWN_SWEEP_PREFIX_SCAN);
	_resetPositionShader = ShaderList::getInstance()->getComputeShader(RendEnum::RESET_LAST_POSITION_PREFIX_SCAN);
	_reallocatePositionShader = ShaderList::getInstance()->getComputeShader(RendEnum::REALLOCATE_RADIX_SORT);

    // Buffers 
    _verticesSSBO = ComputeShader::setWriteBuffer(vec4(), maxNumPoints, GL_DYNAMIC_DRAW);
    _supportVerticesSSBO = ComputeShader::setWriteBuffer(vec4(), _numThreads * 12, GL_DYNAMIC_DRAW);
    _numVerticesSSBO = ComputeShader::setWriteBuffer(unsigned(), 1, GL_DYNAMIC_DRAW);

    _edgeTableSSBO = ComputeShader::setReadBuffer(_edgeTable, 256, GL_STATIC_DRAW);
    _triangleTableSSBO = ComputeShader::setReadBuffer(_triangleTable, 256 * 16, GL_STATIC_DRAW);

	_mortonCodeSSBO = ComputeShader::setWriteBuffer(unsigned(), maxNumPoints, GL_DYNAMIC_DRAW);
	_indicesBufferID_1 = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);
	_indicesBufferID_2 = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);					// Substitutes indicesBufferID_1 for the next iteration
	_pBitsBufferID = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);
	_nBitsBufferID = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);
	_positionBufferID = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);

	_vertexSSBO = ComputeShader::setWriteBuffer(Model3D::VertexGPUData(), maxNumPoints, GL_DYNAMIC_DRAW);
	_faceSSBO = ComputeShader::setWriteBuffer(Model3D::FaceGPUData(), maxNumPoints / 3, GL_DYNAMIC_DRAW);

    float* gridData = (float*) malloc(sizeof(float) * _numThreads);
    for (int x = 0; x < _numDivs.x; ++x)
        for (int y = 0; y < _numDivs.y; ++y)
            for (int z = 0; z < _numDivs.z; ++z)
                gridData[x * _numDivs.y * _numDivs.z + y * _numDivs.z + z] = VOXEL_FREE;

    for (int x = 1; x < _numDivs.x - 1; ++x)
        for (int y = 1; y < _numDivs.y - 1; ++y)
            for (int z = 1; z < _numDivs.z - 1; ++z)
                gridData[x * _numDivs.y * _numDivs.z + y * _numDivs.z + z] = regularGrid.at(x - 1, y - 1, z - 1);

    _gridSSBO = ComputeShader::setReadBuffer(gridData, _numThreads, GL_STATIC_DRAW);
    free(gridData);
}

MarchingCubes::~MarchingCubes()
{
    GLuint buffers[] = { 
		_verticesSSBO, _edgeTableSSBO, _triangleTableSSBO, _supportVerticesSSBO, _numVerticesSSBO, _mortonCodeSSBO,
		_indicesBufferID_1, _indicesBufferID_2, _pBitsBufferID, _nBitsBufferID, _positionBufferID, _vertexSSBO, _faceSSBO
	};
    glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);

	delete[] _indices;
}

void MarchingCubes::triangulateFieldGPU(GLuint gridSSBO, const uvec3& numDivs, float targetValue, std::vector<Triangle3D>& triangles)
{
	this->resetCounter();

    _marchingCubesShader->bindBuffers(std::vector<GLuint>{ _gridSSBO, _verticesSSBO, _numVerticesSSBO, _triangleTableSSBO, _edgeTableSSBO, _supportVerticesSSBO });
    _marchingCubesShader->use();
    _marchingCubesShader->setUniform("gridDims", _numDivs);
    _marchingCubesShader->setUniform("isolevel", 0.5f);
    _marchingCubesShader->setUniform("targetValue", int(targetValue));
    _marchingCubesShader->execute(_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

    unsigned numVertices = *ComputeShader::readData(_numVerticesSSBO, unsigned());
    vec4* vertexData = ComputeShader::readData(_verticesSSBO, vec4());

	this->calculateMortonCodes(numVertices);
	this->sortMortonCodes(numVertices);
	this->fuseSimilarVertices(numVertices);
	this->buildMarchingCubesFaces(numVertices);

    //triangles.resize(numVertices / 3);
    //for (int idx = 0; idx < numVertices; idx += 3)
    //{
    //    triangles[idx / 3] = Triangle3D(vertexData[idx + 0], vertexData[idx + 1], vertexData[idx + 2]);
    //}
}

// [Protected methods]

void MarchingCubes::buildMarchingCubesFaces(unsigned numVertices)
{
	this->resetCounter();

	_buildMarchingCubesShader->bindBuffers(std::vector<GLuint> { _indicesBufferID_2, _indicesBufferID_1, _faceSSBO, _numVerticesSSBO });
	_buildMarchingCubesShader->use();
	_buildMarchingCubesShader->setUniform("numPoints", numVertices);
	_buildMarchingCubesShader->execute(_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	Model3D::FaceGPUData* faceData = ComputeShader::readData(_faceSSBO, Model3D::FaceGPUData());
	std::vector<Model3D::FaceGPUData> faces = std::vector<Model3D::FaceGPUData>(faceData, faceData + numVertices / 3);
};

void MarchingCubes::calculateMortonCodes(unsigned numVertices)
{
	_computeMortonShader->bindBuffers(std::vector<GLuint> { _verticesSSBO, _mortonCodeSSBO });
	_computeMortonShader->use();
	_computeMortonShader->setUniform("numPoints", numVertices);
	_computeMortonShader->setUniform("sceneMaxBoundary", vec3(_numDivs));
	_computeMortonShader->setUniform("sceneMinBoundary", vec3(.0f));
	_computeMortonShader->execute(_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
}

void MarchingCubes::fuseSimilarVertices(unsigned numVertices)
{
	this->resetCounter();

	_fuseSimilarVerticesShader->bindBuffers(std::vector<GLuint> { _indicesBufferID_2, _indicesBufferID_1, _verticesSSBO, _vertexSSBO, _numVerticesSSBO });
	_fuseSimilarVerticesShader->use();
	_fuseSimilarVerticesShader->setUniform("numPoints", numVertices);
	_fuseSimilarVerticesShader->execute(_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	unsigned n = *ComputeShader::readData(_numVerticesSSBO, unsigned());
}

void MarchingCubes::resetCounter()
{
	unsigned zero = 0;
	ComputeShader::updateReadBuffer(_numVerticesSSBO, &zero, 1, GL_DYNAMIC_DRAW);
}

void MarchingCubes::sortMortonCodes(unsigned numVertices)
{
	const unsigned numBits	= 30;	// 10 bits per coordinate (3D)
	unsigned arraySize		= numVertices, currentBits = 0;
	const int numGroups		= ComputeShader::getNumGroups(arraySize);
	const int maxGroupSize	= ComputeShader::getMaxGroupSize();

	// Binary tree parameters
	const unsigned startThreads		= std::ceil(arraySize / 2.0f);
	const int numExec				= std::ceil(std::log2(arraySize));
	const int numGroups2Log			= ComputeShader::getNumGroups(startThreads);
	unsigned numThreads				= 0, iteration;

	ComputeShader::updateReadBuffer(_indicesBufferID_2, _indices, arraySize);

	while (currentBits < numBits)
	{
		std::vector<GLuint> threadCount{ startThreads };
		threadCount.reserve(numExec);

		std::swap(_indicesBufferID_1, _indicesBufferID_2);							// indicesBufferID_2 is initialized with indices cause it's swapped here

		// FIRST STEP: BIT MASK, check if a morton code gives zero or one for a certain mask (iteration)
		unsigned bitMask = 1 << currentBits++;

		_bitMaskShader->bindBuffers(std::vector<GLuint> { _mortonCodeSSBO, _indicesBufferID_1, _pBitsBufferID, _nBitsBufferID });
		_bitMaskShader->use();
		_bitMaskShader->setUniform("arraySize", arraySize);
		_bitMaskShader->setUniform("bitMask", bitMask);
		_bitMaskShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);

		// SECOND STEP: build a binary tree with a summatory of the array
		_reduceShader->bindBuffers(std::vector<GLuint> { _nBitsBufferID });
		_reduceShader->use();
		_reduceShader->setUniform("arraySize", arraySize);

		iteration = 0;
		while (iteration < numExec)
		{
			numThreads = threadCount[threadCount.size() - 1];

			_reduceShader->setUniform("iteration", iteration++);
			_reduceShader->setUniform("numThreads", numThreads);
			_reduceShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);

			threadCount.push_back(std::ceil(numThreads / 2.0f));
		}

		// THIRD STEP: set last position to zero, its faster to do it in GPU than retrieve the array in CPU, modify and write it again to GPU
		_resetPositionShader->bindBuffers(std::vector<GLuint> { _nBitsBufferID });
		_resetPositionShader->use();
		_resetPositionShader->setUniform("arraySize", arraySize);
		_resetPositionShader->execute(1, 1, 1, 1, 1, 1);

		// FOURTH STEP: build tree back to first level and compute position of each bit
		_downSweepShader->bindBuffers(std::vector<GLuint> { _nBitsBufferID });
		_downSweepShader->use();
		_downSweepShader->setUniform("arraySize", arraySize);

		iteration = threadCount.size() - 2;
		while (iteration >= 0 && iteration < numExec)
		{
			_downSweepShader->setUniform("iteration", iteration);
			_downSweepShader->setUniform("numThreads", threadCount[iteration--]);
			_downSweepShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);
		}

		_reallocatePositionShader->bindBuffers(std::vector<GLuint> { _pBitsBufferID, _nBitsBufferID, _indicesBufferID_1, _indicesBufferID_2 });
		_reallocatePositionShader->use();
		_reallocatePositionShader->setUniform("arraySize", arraySize);
		_reallocatePositionShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);
	}

	GLuint* data = ComputeShader::readData(_indicesBufferID_2, GLuint());
	std::vector<GLuint> dataBuffer = std::vector<GLuint>(data, data + arraySize);
}