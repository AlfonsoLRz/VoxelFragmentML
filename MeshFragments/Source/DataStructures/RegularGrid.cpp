#include "stdafx.h"
#include "RegularGrid.h"

#include "Geometry/3D/AABB.h"
#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/MarchingCubes.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/Tetravoxelizer.h"
#include "Graphics/Core/Voronoi.h"
#include "tinyply.h"
#include "Utilities/ChronoUtilities.h"
#include "VoxWriter.h"

/// Public methods

RegularGrid::RegularGrid(const AABB& aabb, const ivec3& subdivisions) :
	_aabb(aabb), _marchingCubes(nullptr), _numDivs(subdivisions)
{
	this->setAABB(aabb, _numDivs);
	this->buildGrid();
	this->getComputeShaders();
}

RegularGrid::RegularGrid(const ivec3& subdivisions) : _cellSize(.0f), _marchingCubes(nullptr), _numDivs(subdivisions)
{
	this->buildGrid();
	this->getComputeShaders();
}

RegularGrid::~RegularGrid()
{
	delete _marchingCubes;
	glDeleteBuffers(1, &_countSSBO);
	glDeleteBuffers(1, &_ssbo);
}

unsigned RegularGrid::calculateMaxQuadrantOccupancy(unsigned subdivisions)
{
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
	unsigned numGroups = ComputeShader::getNumGroups(numCells);
	uvec3 step = glm::ceil(vec3(_numDivs) / vec3(subdivisions));

	this->resetBuffer(_countSSBO, unsigned(0), numCells);

	_countQuadrantOccupancyShader->bindBuffers(std::vector<GLuint>{ _ssbo, _countSSBO });
	_countQuadrantOccupancyShader->use();
	_countQuadrantOccupancyShader->setUniform("gridDims", numDivs);
	_countQuadrantOccupancyShader->setUniform("numCells", numCells);
	_countQuadrantOccupancyShader->setUniform("step", step);
	_countQuadrantOccupancyShader->setUniform("subdivisions", subdivisions);
	_countQuadrantOccupancyShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	unsigned* countData = ComputeShader::readData(_countSSBO, unsigned());
	unsigned maxCount = *std::max_element(countData, countData + subdivisions * subdivisions * subdivisions);

	return maxCount;
}

void RegularGrid::detectBoundaries(int boundarySize)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::DETECT_BOUNDARIES);

	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
	unsigned numGroups = ComputeShader::getNumGroups(numCells);

	shader->bindBuffers(std::vector<GLuint>{ _ssbo });
	shader->use();
	shader->setUniform("boundarySize", boundarySize);
	shader->setUniform("gridDims", numDivs);
	shader->setUniform("numCells", numCells);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	CellGrid* gridData = ComputeShader::readData(_ssbo, CellGrid());
	std::copy(gridData, gridData + numCells, _grid.begin());
}

void RegularGrid::erode(FractureParameters::ErosionType fractureParams, uint32_t convolutionSize, uint8_t numIterations, float erosionProbability, float erosionThreshold)
{
	if (!(convolutionSize % 2))
		++convolutionSize;

	uint32_t maskSize = convolutionSize * convolutionSize * convolutionSize, convolutionCenter = std::floor(convolutionSize / 2.0f), activations = 0;
	float* erosionMask = (float*)calloc(maskSize, sizeof(float));

	if (fractureParams == FractureParameters::SQUARE)
	{
		std::fill(erosionMask, erosionMask + maskSize, 1);
		activations = maskSize;
	}
	else if (fractureParams == FractureParameters::CROSS)
	{
		for (int x = 0; x < convolutionSize; ++x)
			erosionMask[x * convolutionSize * convolutionSize + convolutionCenter * convolutionSize + convolutionCenter] = 1.0f;
		for (int y = 0; y < convolutionSize; ++y)
			erosionMask[convolutionCenter * convolutionSize * convolutionSize + y * convolutionSize + convolutionCenter] = 1.0f;
		for (int z = 0; z < convolutionSize; ++z)
			erosionMask[convolutionCenter * convolutionSize * convolutionSize + convolutionCenter * convolutionSize + z] = 1.0f;
		activations = 1.0f / 3.0f * maskSize;
	}
	else if (fractureParams == FractureParameters::ELLIPSE)
	{
		for (int x = 0; x < convolutionSize; ++x)
			for (int y = 0; y < convolutionSize; ++y)
				for (int z = 0; z < convolutionSize; ++z)
				{
					if (glm::distance(vec3(x, y, z), vec3(convolutionCenter)) < convolutionCenter + glm::epsilon<float>())
					{
						erosionMask[x * convolutionSize * convolutionSize + y * convolutionSize + z] = 1.0f;
						++activations;
					}
				}
	}

	// Noise
	std::vector<float> noiseBuffer;
	this->fillNoiseBuffer(noiseBuffer, 1e6);

	// Input data
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
	unsigned numGroups = ComputeShader::getNumGroups(numCells);
	const GLuint maskSSBO = ComputeShader::setReadBuffer(&erosionMask[0], maskSize, GL_STATIC_DRAW);
	const GLuint noiseSSBO = ComputeShader::setReadBuffer(&noiseBuffer[0], noiseBuffer.size(), GL_STATIC_DRAW);

	for (int idx = 0; idx < numIterations; ++idx)
	{
		_erodeShader->bindBuffers(std::vector<GLuint>{ _ssbo, maskSSBO, noiseSSBO });
		_erodeShader->use();
		_erodeShader->setUniform("numActivations", activations);
		_erodeShader->setUniform("gridDims", numDivs);
		_erodeShader->setUniform("maskSize", convolutionSize);
		_erodeShader->setUniform("maskSize2", unsigned(std::floor(convolutionSize / 2.0f)));
		_erodeShader->setUniform("numCells", numCells);
		_erodeShader->setUniform("noiseBufferSize", static_cast<unsigned>(noiseBuffer.size()));
		_erodeShader->setUniform("erosionProbability", erosionProbability);
		_erodeShader->setUniform("erosionThreshold", erosionThreshold);
		_erodeShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
	}

	CellGrid* gridData = ComputeShader::readData(_ssbo, CellGrid());
	std::copy(gridData, gridData + numCells, _grid.begin());

	GLuint buffers[] = { maskSSBO, noiseSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

void RegularGrid::exportGrid()
{
	vox::VoxWriter vox;
	vec3 size = _aabb.size();
	float minSize = glm::min(glm::min(size.x, size.y), size.z);
	size /= minSize;
	unsigned maxDimension = glm::max(glm::max(_numDivs.x, _numDivs.y), _numDivs.z);
	uvec3 offset = uvec3(vec3(maxDimension) * size - vec3(_numDivs));
	uvec3 halfOffset = offset / uvec3(2);
	uvec3 otherOffset = offset - halfOffset;
	halfOffset.y = 0; otherOffset.y = offset.y;

	for (int x = 0; x < _numDivs.x; ++x)
	{
		unsigned positionIndex;

		for (int y = 0; y < _numDivs.y; ++y)
		{
			for (int z = 0; z < _numDivs.z; ++z)
			{
				positionIndex = this->getPositionIndex(x, y, z);
				if (_grid[positionIndex]._value > VOXEL_FREE)
					vox.AddVoxel(x + halfOffset.x, z + halfOffset.z, y, _grid[positionIndex]._value);
			}
		}
	}

	vox.SaveToFile("grid" + std::to_string(RandomUtilities::getUniformRandomInt(0, 10e5)) + ".vox");
	vox.PrintStats();
}

void RegularGrid::fill(Model3D::ModelComponent* modelComponent)
{
	Tetravoxelizer tetravoxelizer;
	tetravoxelizer.initialize(_numDivs);
	tetravoxelizer.initializeModel(modelComponent->_geometry, modelComponent->_topology, _aabb);
	tetravoxelizer.compute(_voxelOpenGL);
	tetravoxelizer.deleteModelResources();
	tetravoxelizer.deleteResources();

#pragma omp parallel for
	for (int y = 0; y < _numDivs.y; ++y)
	{
		glm::uint positionIndex;
		for (int x = 0; x < _numDivs.x; ++x)
		{
			for (int z = 0; z < _numDivs.z; ++z)
			{
				positionIndex = y * _numDivs.x * _numDivs.z + z * _numDivs.x + x;
				if (_voxelOpenGL[positionIndex] == 1)
				{
					this->set(x, y, z, VOXEL_FREE);
				}
			}
		}
	}

	this->updateSSBO();
}

void RegularGrid::fill(const Voronoi& voronoi)
{
#pragma omp parallel for
	for (int x = 0; x < _numDivs.x; ++x)
	{
		int cluster;
		unsigned positionIndex;

		for (int y = 0; y < _numDivs.y; ++y)
		{
			for (int z = 0; z < _numDivs.z; ++z)
			{
				positionIndex = this->getPositionIndex(x, y, z);

				if (_grid[positionIndex]._value == VOXEL_FREE)
				{
					cluster = voronoi.getCluster(x, y, z);
					_grid[positionIndex]._value = cluster != -1 ? cluster + (VOXEL_FREE + 1) : _grid[positionIndex]._value;
				}
			}
		}
	}
}

void RegularGrid::fillNoiseBuffer(std::vector<float>& noiseBuffer, unsigned numSamples)
{
	noiseBuffer.resize(numSamples);

#pragma omp parallel for
	for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
	{
		noiseBuffer[sampleIdx] = RandomUtilities::getUniformRandom(.0f, 1.0f);
	}
}

void RegularGrid::getAABBs(std::vector<AABB>& aabb)
{
	vec3 max, min;

	for (int x = 0; x < _numDivs.x; ++x)
	{
		for (int y = 0; y < _numDivs.y; ++y)
		{
			for (int z = 0; z < _numDivs.z; ++z)
			{
				if (_grid[this->getPositionIndex(x, y, z)]._value != VOXEL_EMPTY)
				{
					min = _aabb.min() + _cellSize * vec3(x, y, z);
					max = min + _cellSize;

					aabb.push_back(AABB(min, max));
				}
			}
		}
	}
}

void RegularGrid::insertPoint(const vec3& position, unsigned index)
{
	uvec3 gridIndex = getPositionIndex(position);

	_grid[this->getPositionIndex(gridIndex.x, gridIndex.y, gridIndex.z)]._value = index;
}

unsigned RegularGrid::numOccupiedVoxels()
{
	unsigned count = 0;
	for (const CellGrid& grid : _grid)
		count += grid._value > VOXEL_FREE;

	return count;
}

void RegularGrid::queryCluster(
	const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, std::vector<float>& clusterIdx,
	std::vector<unsigned>& boundaryFaces, std::vector<std::unordered_map<unsigned, float>>& faceClusterOccupancy)
{
	std::unordered_map<uint16_t, unsigned> values;
	faceClusterOccupancy.resize(faces.size());

	size_t numFragments = this->countValues(values);
	size_t numSamples = 1000;
	size_t actualSize = numFragments * faces.size();
	size_t maxFaces = std::min(faces.size(), static_cast<size_t>(std::floor(ComputeShader::getMaxSSBOSize(sizeof(GLuint)) / numFragments)));
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numGroups = ComputeShader::getNumGroups(maxFaces * numSamples);
	std::vector<float> noiseBuffer;
	this->fillNoiseBuffer(noiseBuffer, numSamples * numSamples);

	ComputeShader::getMaxSSBOSize(sizeof(unsigned));

	// Buffers for counting
	GLuint* count = (GLuint*)malloc(maxFaces * numFragments * sizeof(GLuint));
	clusterIdx.resize(faces.size()); std::fill(clusterIdx.begin(), clusterIdx.end(), -1.0f);
	GLuint* boundary = (GLuint*)calloc(maxFaces * numFragments, sizeof(unsigned));

	const GLuint countSSBO = ComputeShader::setReadBuffer(count, maxFaces * numFragments, GL_DYNAMIC_DRAW);
	const GLuint vertexSSBO = ComputeShader::setReadBuffer(vertices, GL_STATIC_DRAW);
	const GLuint gridSSBO = ComputeShader::setReadBuffer(_grid, GL_STATIC_DRAW);
	const GLuint boundarySSBO = ComputeShader::setReadBuffer(boundary, maxFaces * numFragments, GL_DYNAMIC_DRAW);
	const GLuint noiseSSBO = ComputeShader::setReadBuffer(noiseBuffer, GL_STATIC_DRAW);
	const GLuint clusterSSBO = ComputeShader::setReadBuffer(clusterIdx, GL_DYNAMIC_DRAW);

	size_t numProcessedFaces = 0;
	while (numProcessedFaces < faces.size())
	{
		unsigned currentNumFaces = std::min(faces.size() - numProcessedFaces, maxFaces);
		const GLuint faceSSBO = ComputeShader::setReadBuffer(faces.data() + numProcessedFaces, currentNumFaces, GL_DYNAMIC_DRAW);
		std::fill(count, count + currentNumFaces * numFragments, 0);
		ComputeShader::updateReadBufferSubset(countSSBO, count, 0, currentNumFaces * numFragments);
		ComputeShader::updateReadBufferSubset(boundarySSBO, boundary, 0, currentNumFaces * numFragments);

		_countVoxelTriangleShader->bindBuffers(std::vector<GLuint>{ vertexSSBO, faceSSBO, gridSSBO, countSSBO, boundarySSBO, noiseSSBO });
		_countVoxelTriangleShader->use();
		_countVoxelTriangleShader->setUniform("aabbMin", _aabb.min());
		_countVoxelTriangleShader->setUniform("cellSize", _cellSize);
		_countVoxelTriangleShader->setUniform("gridDims", numDivs);
		_countVoxelTriangleShader->setUniform("numFragments", GLuint(numFragments));
		_countVoxelTriangleShader->setUniform("numSamples", GLuint(numSamples));
		_countVoxelTriangleShader->setUniform("numFaces", GLuint(faces.size()));
		_countVoxelTriangleShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		_pickVoxelTriangleShader->bindBuffers(std::vector<GLuint>{ countSSBO, boundarySSBO, clusterSSBO });
		_pickVoxelTriangleShader->use();
		_pickVoxelTriangleShader->setUniform("numFragments", GLuint(numFragments));
		_pickVoxelTriangleShader->setUniform("numFaces", GLuint(faces.size()));
		_pickVoxelTriangleShader->execute(ComputeShader::getNumGroups(currentNumFaces), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		// Extract clusters associated to each face
		GLuint* countData = ComputeShader::readData(countSSBO, GLuint());

#pragma omp parallel for
		for (int faceIdx = 0; faceIdx < currentNumFaces; ++faceIdx)
		{
			for (int clusterIdx = 0; clusterIdx < numFragments; ++clusterIdx)
			{
				if (countData[faceIdx * numFragments + clusterIdx])
				{
					faceClusterOccupancy.at(faceIdx + numProcessedFaces)[clusterIdx] = countData[faceIdx * numFragments + clusterIdx];
				}
			}

			for (auto& pair : faceClusterOccupancy[faceIdx])
			{
				pair.second /= numSamples;
			}
		}

		numProcessedFaces += currentNumFaces;

		glDeleteBuffers(1, &faceSSBO);
	}

	float* clusterData = ComputeShader::readData(clusterSSBO, float());
	clusterIdx = std::vector<float>(clusterData, clusterData + faces.size());

	for (int idx = 0; idx < clusterIdx.size(); ++idx)
	{
		if (clusterIdx[idx] < .0f)
		{
			boundaryFaces.push_back(idx);
			clusterIdx[idx] = -clusterIdx[idx];
		}
	}

	GLuint buffers[] = { countSSBO, vertexSSBO, gridSSBO, boundarySSBO, noiseSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);

	free(count);
	free(boundary);
}

void RegularGrid::queryCluster(std::vector<vec4>* points, std::vector<float>& clusterIdx)
{
	// Input data
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numThreads = points->size();
	unsigned numGroups = ComputeShader::getNumGroups(numThreads);

	// Input data
	const GLuint vertexSSBO = ComputeShader::setReadBuffer(*points, GL_STATIC_DRAW);
	const GLuint gridSSBO = ComputeShader::setReadBuffer(_grid, GL_STATIC_DRAW);
	const GLuint clusterSSBO = ComputeShader::setWriteBuffer(float(), points->size(), GL_DYNAMIC_DRAW);

	_assignVertexClusterShader->bindBuffers(std::vector<GLuint>{ vertexSSBO, gridSSBO, clusterSSBO });
	_assignVertexClusterShader->use();
	_assignVertexClusterShader->setUniform("aabbMin", _aabb.min());
	_assignVertexClusterShader->setUniform("cellSize", _cellSize);
	_assignVertexClusterShader->setUniform("gridDims", numDivs);
	_assignVertexClusterShader->setUniform("numVertices", GLuint(points->size()));
	_assignVertexClusterShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	float* clusterData = ComputeShader::readData(clusterSSBO, float());
	clusterIdx = std::vector<float>(clusterData, clusterData + points->size());

	GLuint buffers[] = { vertexSSBO, gridSSBO, clusterSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

void RegularGrid::resetFilling()
{
	size_t numCells = _grid.size();
#pragma omp parallel for
	for (int idx = 0; idx < numCells; ++idx)
		_grid[idx]._value = glm::clamp(_grid[idx]._value, uint16_t(VOXEL_EMPTY), uint16_t(VOXEL_FREE + 1));
}

void RegularGrid::resetMarchingCubes()
{
	delete _marchingCubes;
	_marchingCubes = new MarchingCubes(*this, 1, _numDivs, 5);
}

void RegularGrid::setAABB(const AABB& aabb, const ivec3& gridDims)
{
	//vec3 aabbSize = aabb.size();
	//float maxDimension = glm::max(glm::max(aabbSize.x, aabbSize.y), aabbSize.z);
	//vec3 scale = vec3(maxDimension) / aabbSize;
	//aabbSize *= scale;
	//_aabb = AABB(_aabb.min() * scale, _aabb.min() * scale + aabbSize);
	//unsigned maxDivs = glm::max(glm::max(_numDivs.x, _numDivs.y), _numDivs.z);
	//_numDivs = glm::ceil(vec3(maxDivs) * aabbSize / maxDimension);

	_aabb = AABB(aabb);
	_numDivs = gridDims;
	_cellSize = _aabb.size() / vec3(_numDivs);

	this->cleanGrid();
}

std::vector<Model3D*> RegularGrid::toTriangleMesh(FractureParameters& fractParameters, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata)
{
	std::vector<Model3D*> meshes;
	std::unordered_map<uint16_t, unsigned> valuesSet;
	std::vector<uint16_t> values;
	unsigned globalCount = 0;

	this->countValues(valuesSet);
	meshes.resize(valuesSet.size());

	values.reserve(valuesSet.size());
	fragmentMetadata.resize(valuesSet.size());
	for (auto it = valuesSet.begin(); it != valuesSet.end(); )
	{
		fragmentMetadata[it->first - (VOXEL_FREE + 1)]._voxels = it->second;

		globalCount += it->second;
		values.push_back(std::move(valuesSet.extract(it++).key()));
	}

#pragma omp parallel for
	for (int idx = 0; idx < values.size(); ++idx)
	{
		fragmentMetadata[idx]._id = idx;
		fragmentMetadata[idx]._percentage = fragmentMetadata[idx]._voxels / static_cast<float>(globalCount);
		fragmentMetadata[idx]._occupiedVoxels = globalCount;
		fragmentMetadata[idx]._voxelizationSize = _numDivs;
	}

	std::sort(values.begin(), values.end());

	if (_marchingCubes)
		_marchingCubes->setGrid(*this);

	vec3 scale = (_aabb.size()) / vec3(_numDivs);
	vec3 minPoint = _aabb.min();
	mat4 transformationMatrix = glm::translate(glm::mat4(1.0f), -vec3(1.0f) * scale) * glm::translate(glm::mat4(1.0f), minPoint) * glm::scale(glm::mat4(1.0f), scale);

	for (int idx = 0; idx < values.size(); ++idx)
		meshes[idx] = _marchingCubes->triangulateFieldGPU(_ssbo, values[idx], fractParameters, transformationMatrix);

	return meshes;
}

void RegularGrid::undoMask()
{
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
	unsigned numGroups = ComputeShader::getNumGroups(numCells);

	_undoMaskShader->bindBuffers(std::vector<GLuint>{ _ssbo });
	_undoMaskShader->use();
	_undoMaskShader->setUniform("numCells", numCells);
	_undoMaskShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	CellGrid* gridData = ComputeShader::readData(_ssbo, CellGrid());
	std::copy(gridData, gridData + numCells, _grid.begin());
}

void RegularGrid::updateSSBO()
{
	ComputeShader::updateReadBufferSubset(_ssbo, _grid.data(), 0, _grid.size());
}

// [Protected methods]

RegularGrid::CellGrid* RegularGrid::data()
{
	return _grid.data();
}

uint16_t RegularGrid::at(int x, int y, int z) const
{
	return _grid[this->getPositionIndex(x, y, z)]._value;
}

glm::uvec3 RegularGrid::getNumSubdivisions() const
{
	return _numDivs;
}

void RegularGrid::homogenize()
{
#pragma omp parallel for
	for (int x = 0; x < _numDivs.x; ++x)
		for (int y = 0; y < _numDivs.y; ++y)
			for (int z = 0; z < _numDivs.z; ++z)
				if (this->at(x, y, z) != VOXEL_EMPTY)
					this->set(x, y, z, VOXEL_FREE);
}

bool RegularGrid::isOccupied(int x, int y, int z) const
{
	return this->at(x, y, z) != VOXEL_EMPTY;
}

bool RegularGrid::isEmpty(int x, int y, int z) const
{
	return this->at(x, y, z) == VOXEL_EMPTY;
}

size_t RegularGrid::length() const
{
	return _numDivs.x * _numDivs.y * _numDivs.z;
}

void RegularGrid::set(int x, int y, int z, uint8_t i)
{
	_grid[this->getPositionIndex(x, y, z)]._value = i;
}

/// Protected methods	

void RegularGrid::buildGrid()
{
	_grid = std::vector<CellGrid>(_numDivs.x * _numDivs.y * _numDivs.z, CellGrid());
	_ssbo = ComputeShader::setReadBuffer(_grid.data(), _grid.size(), GL_DYNAMIC_DRAW);
	_countSSBO = ComputeShader::setWriteBuffer(GLuint(), _numDivs.x * _numDivs.y * _numDivs.z, GL_DYNAMIC_DRAW);
	_voxelOpenGL = std::vector<unsigned char>(_numDivs.x * _numDivs.y * _numDivs.z, 0);
}

void RegularGrid::cleanGrid()
{
	//_grid = std::vector<CellGrid>(_numDivs.x * _numDivs.y * _numDivs.z);
	std::fill(_grid.begin(), _grid.end(), CellGrid());
	std::fill(_voxelOpenGL.begin(), _voxelOpenGL.end(), 0);

	ComputeShader::updateReadBufferSubset(_ssbo, _grid.data(), 0, _grid.size());
}

size_t RegularGrid::countValues(std::unordered_map<uint16_t, unsigned>& values)
{
	unsigned index;
	uint16_t value;
	auto it = values.begin();

	for (unsigned int x = 0; x < _numDivs.x; ++x)
		for (unsigned int y = 0; y < _numDivs.y; ++y)
			for (unsigned int z = 0; z < _numDivs.z; ++z)
			{
				index = this->getPositionIndex(x, y, z);
				if (_grid[index]._value > VOXEL_FREE)
				{
					value = this->unmask(_grid[index]._value);

					it = values.find(value);
					if (it != values.end())
						++it->second;
					else
						values.insert(std::make_pair(value, 1));
				}
			}

	return values.size();
}

void RegularGrid::getComputeShaders()
{
	_assignVertexClusterShader = ShaderList::getInstance()->getComputeShader(RendEnum::ASSIGN_VERTEX_CLUSTER);
	_countQuadrantOccupancyShader = ShaderList::getInstance()->getComputeShader(RendEnum::COUNT_QUADRANT_OCCUPANCY);
	_countVoxelTriangleShader = ShaderList::getInstance()->getComputeShader(RendEnum::COUNT_VOXEL_TRIANGLE);
	_erodeShader = ShaderList::getInstance()->getComputeShader(RendEnum::ERODE_GRID);
	_pickVoxelTriangleShader = ShaderList::getInstance()->getComputeShader(RendEnum::SELECT_VOXEL_TRIANGLE);
	_resetCounterShader = ShaderList::getInstance()->getComputeShader(RendEnum::RESET_BUFFER);
	_undoMaskShader = ShaderList::getInstance()->getComputeShader(RendEnum::UNDO_MASK_SHADER);
}

uvec3 RegularGrid::getPositionIndex(const vec3& position)
{
	unsigned x = (position.x - _aabb.min().x) / _cellSize.x, y = (position.y - _aabb.min().y) / _cellSize.y, z = (position.z - _aabb.min().z) / _cellSize.z;
	unsigned zeroUnsigned = 0;

	return uvec3(glm::clamp(x, zeroUnsigned, _numDivs.x - 1), glm::clamp(y, zeroUnsigned, _numDivs.y - 1), glm::clamp(z, zeroUnsigned, _numDivs.z - 1));
}

unsigned RegularGrid::getPositionIndex(int x, int y, int z) const
{
	return x * _numDivs.y * _numDivs.z + y * _numDivs.z + z;
}

void RegularGrid::resetBuffer(GLuint ssbo, unsigned value, unsigned count)
{
	_resetCounterShader->bindBuffers(std::vector<GLuint>{ ssbo });
	_resetCounterShader->use();
	_resetCounterShader->setUniform("arraySize", count);
	_resetCounterShader->setUniform("value", value);
	_resetCounterShader->execute(ComputeShader::getNumGroups(count), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
}

uint16_t RegularGrid::unmask(uint16_t value) const
{
	return value & uint16_t(~(1 << MASK_POSITION));
}

unsigned RegularGrid::getPositionIndex(int x, int y, int z, const uvec3& numDivs)
{
	return x * numDivs.y * numDivs.z + y * numDivs.z + z;
}
