#include "stdafx.h"
#include "RegularGrid.h"

#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/MarchingCubes.h"
#include "tinyply/tinyply.h"
#include "VoxWriter.h"

/// Public methods

RegularGrid::RegularGrid(const AABB& aabb, uvec3 subdivisions) :
	_aabb(aabb), _numDivs(subdivisions)
{
	vec3 aabbSize = _aabb.size();
	float maxDimension = glm::max(glm::max(aabbSize.x, aabbSize.y), aabbSize.z);
	vec3 scale = vec3(maxDimension) / aabbSize;
	aabbSize *= scale;
	_aabb = AABB(_aabb.min() * scale, _aabb.min() * scale + aabbSize);
	_cellSize = _aabb.size() / vec3(_numDivs);

	//_cellSize = vec3((_aabb.max().x - _aabb.min().x) / float(subdivisions.x), (_aabb.max().y - _aabb.min().y) / float(subdivisions.y), (_aabb.max().z - _aabb.min().z) / float(subdivisions.z));
	//float minCellSize = glm::min(_cellSize.x, glm::min(_cellSize.y, _cellSize.z));
	//vec3 scale = _cellSize / minCellSize;
	//_cellSize = vec3(minCellSize);
	//_numDivs = glm::ceil(vec3(_numDivs) * scale);

	this->buildGrid();
}

RegularGrid::RegularGrid(uvec3 subdivisions) : _cellSize(.0f), _numDivs(subdivisions)
{
	
}

RegularGrid::~RegularGrid()
{
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
	_grid = std::vector<CellGrid>(gridData, gridData + numCells);
}

void RegularGrid::erode(FractureParameters::ErosionType fractureParams, uint32_t convolutionSize, uint8_t numIterations, float erosionProbability, float erosionThreshold)
{
	ComputeShader* erodeShader = ShaderList::getInstance()->getComputeShader(RendEnum::ERODE_GRID);
	ComputeShader* undoMaskShader = ShaderList::getInstance()->getComputeShader(RendEnum::UNDO_MASK_SHADER);

	if (!(convolutionSize % 2))
		++convolutionSize;

	uint32_t maskSize = convolutionSize * convolutionSize * convolutionSize, convolutionCenter = std::floor(convolutionSize / 2.0f), activations = 0;
	float* erosionMask = (float*) calloc(maskSize, sizeof(float));

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
		erodeShader->bindBuffers(std::vector<GLuint>{ _ssbo, maskSSBO, noiseSSBO });
		erodeShader->use();
		erodeShader->setUniform("numActivations", activations);
		erodeShader->setUniform("gridDims", numDivs);
		erodeShader->setUniform("maskSize", convolutionSize);
		erodeShader->setUniform("maskSize2", unsigned(std::floor(convolutionSize / 2.0f)));
		erodeShader->setUniform("numCells", numCells);
		erodeShader->setUniform("noiseBufferSize", static_cast<unsigned>(noiseBuffer.size()));
		erodeShader->setUniform("erosionProbability", erosionProbability);
		erodeShader->setUniform("erosionThreshold", erosionThreshold);
		erodeShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
	}

	undoMaskShader->bindBuffers(std::vector<GLuint>{ _ssbo, maskSSBO, noiseSSBO });
	undoMaskShader->use();
	undoMaskShader->setUniform("numCells", numCells);
	undoMaskShader->setUniform("position", unsigned(15));
	undoMaskShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	CellGrid* gridData = ComputeShader::readData(_ssbo, CellGrid());
	_grid = std::vector<CellGrid>(gridData, gridData + numCells);

	//vec4* testData = ComputeShader::readData(testSSBO, vec4());
	//std::vector<vec4> testBuffer = std::vector<vec4>(testData, testData + numCells);

	GLuint buffers[] = { maskSSBO, noiseSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

void RegularGrid::exportGrid(const AABB& aabb)
{
	vox::VoxWriter vox;
	vec3 size = aabb.size();
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

float RegularGrid::fill(Model3D::ModelComponent* modelComponent, bool fill, int numSamples)
{
	ComputeShader* boundaryShader = ShaderList::getInstance()->getComputeShader(RendEnum::BUILD_REGULAR_GRID);
	ComputeShader* fillShader = ShaderList::getInstance()->getComputeShader(RendEnum::FILL_REGULAR_GRID_VOXEL);

	// Input data
	uvec3 numDivs		= this->getNumSubdivisions();
	unsigned numCells	= numDivs.x * numDivs.y * numDivs.z;
	unsigned numThreads = modelComponent->_topology.size() * numSamples;
	unsigned numGroups1	= ComputeShader::getNumGroups(numThreads);
	unsigned numGroups2 = ComputeShader::getNumGroups(numCells);

	// Noise buffer
	std::vector<float> noiseBuffer;
	this->fillNoiseBuffer(noiseBuffer, numSamples * 2);

	// Max. triangle area
	float maxArea = FLT_MIN;
	Triangle3D triangle;
	for (const Model3D::FaceGPUData& face : modelComponent->_topology)
	{
		triangle = Triangle3D(modelComponent->_geometry[face._vertices.x]._position, modelComponent->_geometry[face._vertices.y]._position, modelComponent->_geometry[face._vertices.z]._position);
		maxArea = std::max(maxArea, triangle.area());
	}

	// Input data
	const GLuint noiseSSBO	= ComputeShader::setReadBuffer(noiseBuffer, GL_STATIC_DRAW);

	boundaryShader->bindBuffers(std::vector<GLuint>{ modelComponent->_geometrySSBO, modelComponent->_topologySSBO, noiseSSBO, _ssbo });
	boundaryShader->use();
	boundaryShader->setUniform("aabbMin", _aabb.min());
	boundaryShader->setUniform("cellSize", _cellSize);
	boundaryShader->setUniform("gridDims", numDivs);
	boundaryShader->setUniform("maxArea", maxArea);
	boundaryShader->setUniform("numFaces", GLuint(modelComponent->_topology.size()));
	boundaryShader->setUniform("numSamples", GLuint(numSamples));
	boundaryShader->execute(numGroups1, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	// Fill grid once the boundaries are established
	if (fill)
	{
		fillShader->use();
		fillShader->bindBuffers(std::vector<GLuint>{_ssbo});
		fillShader->setUniform("gridDims", numDivs);
		fillShader->setUniform("numVoxels", numCells);
		fillShader->execute(numGroups2, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
	}

	CellGrid* gridData = ComputeShader::readData(_ssbo, CellGrid());
	_grid = std::vector<CellGrid>(gridData, gridData + numCells);

	GLuint buffers[] = { noiseSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
	
	return maxArea;
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
	ComputeShader* countVoxelTriangle = ShaderList::getInstance()->getComputeShader(RendEnum::COUNT_VOXEL_TRIANGLE);
	ComputeShader* pickVoxelTriangle = ShaderList::getInstance()->getComputeShader(RendEnum::SELECT_VOXEL_TRIANGLE);

	std::unordered_set<uint16_t> values;
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

	//for (const Model3D::FaceGPUData& face : faces)
	//{
	//	std::fill(count, count + numFragments, 0);

	//	for (int idx = 0; idx < numSamples; ++idx)
	//	{
	//		vec3 v1 = vertices[face._vertices.x]._position, v2 = vertices[face._vertices.y]._position, v3 = vertices[face._vertices.z]._position;
	//		vec3 u = v2 - v1, v = v3 - v1;
	//		vec2 randomFactors = vec2(noiseBuffer[idx * 2 + 0], noiseBuffer[idx * 2 + 1]);

	//		if (randomFactors.x + randomFactors.y >= 1.0f)
	//		{
	//			randomFactors = 1.0f - randomFactors;
	//		}

	//		vec3 point = v1 + u * randomFactors.x + v * randomFactors.y;
	//		point.y = ((face._minPoint + face._maxPoint) / 2.0f).y;
	//		uvec3 gridIndex = getPositionIndex(point);
	//		unsigned gridIndexFlat = getPositionIndex(gridIndex.x, gridIndex.y, gridIndex.z);

	//		if (_grid[gridIndexFlat]._value > VOXEL_FREE)
	//			count[_grid[gridIndexFlat]._value] += 1.0f;
	//	}

	//	int maxCount = 0, id = 0;

	//	for (int i = 0; i < numFragments; ++i)
	//	{
	//		if (count[i] > maxCount)
	//		{
	//			maxCount = count[i];
	//			id = i;
	//		}
	//	}

	//	clusterIdx[numProcessedFaces] = id;

	//	++numProcessedFaces;
	//}

	while (numProcessedFaces < faces.size())
	{
		unsigned currentNumFaces = std::min(faces.size() - numProcessedFaces, maxFaces);
		const GLuint faceSSBO = ComputeShader::setReadBuffer(faces.data() + numProcessedFaces, currentNumFaces, GL_DYNAMIC_DRAW);
		std::fill(count, count + currentNumFaces * numFragments, 0);
		ComputeShader::updateReadBuffer(countSSBO, count, currentNumFaces * numFragments, GL_DYNAMIC_DRAW);
		ComputeShader::updateReadBuffer(boundarySSBO, boundary, currentNumFaces * numFragments, GL_DYNAMIC_DRAW);

		countVoxelTriangle->bindBuffers(std::vector<GLuint>{ vertexSSBO, faceSSBO, gridSSBO, countSSBO, boundarySSBO, noiseSSBO });
		countVoxelTriangle->use();
		countVoxelTriangle->setUniform("aabbMin", _aabb.min());
		countVoxelTriangle->setUniform("cellSize", _cellSize);
		countVoxelTriangle->setUniform("gridDims", numDivs);
		countVoxelTriangle->setUniform("numFragments", GLuint(numFragments));
		countVoxelTriangle->setUniform("numSamples", GLuint(numSamples));
		countVoxelTriangle->setUniform("numFaces", GLuint(faces.size()));
		countVoxelTriangle->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		pickVoxelTriangle->bindBuffers(std::vector<GLuint>{ countSSBO, boundarySSBO, clusterSSBO });
		pickVoxelTriangle->use();
		pickVoxelTriangle->setUniform("numFragments", GLuint(numFragments));
		pickVoxelTriangle->setUniform("numFaces", GLuint(faces.size()));
		pickVoxelTriangle->execute(ComputeShader::getNumGroups(currentNumFaces), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

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
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::ASSIGN_VERTEX_CLUSTER);

	// Input data
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numThreads = points->size();
	unsigned numGroups = ComputeShader::getNumGroups(numThreads);

	// Input data
	const GLuint vertexSSBO = ComputeShader::setReadBuffer(*points, GL_STATIC_DRAW);
	const GLuint gridSSBO = ComputeShader::setReadBuffer(_grid, GL_STATIC_DRAW);
	const GLuint clusterSSBO = ComputeShader::setWriteBuffer(float(), points->size(), GL_DYNAMIC_DRAW);

	shader->bindBuffers(std::vector<GLuint>{ vertexSSBO, gridSSBO, clusterSSBO });
	shader->use();
	shader->setUniform("aabbMin", _aabb.min());
	shader->setUniform("cellSize", _cellSize);
	shader->setUniform("gridDims", numDivs);
	shader->setUniform("numVertices", GLuint(points->size()));
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	float* clusterData = ComputeShader::readData(clusterSSBO, float());
	clusterIdx = std::vector<float>(clusterData, clusterData + points->size());

	GLuint buffers[] = { vertexSSBO, gridSSBO, clusterSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

std::vector<Model3D*> RegularGrid::toTriangleMesh()
{
	std::vector<Model3D*> meshes;
	std::unordered_set<uint16_t> valuesSet;
	std::vector<uint16_t> values;

	this->countValues(valuesSet);
	meshes.resize(valuesSet.size());

	values.reserve(valuesSet.size());
	for (auto it = valuesSet.begin(); it != valuesSet.end(); ) 
	{
		values.push_back(std::move(valuesSet.extract(it++).value()));
	}

	MarchingCubes mCubes(*this, _numDivs, 5);
	vec3 scale = (_aabb.size()) / vec3(_numDivs + uvec3(2));
	vec3 minPoint = _aabb.min();
	mat4 transformationMatrix = glm::translate(glm::mat4(1.0f), -vec3(1.0f) * scale) * glm::translate(glm::mat4(1.0f), minPoint) * glm::scale(glm::mat4(1.0f), scale);

	for (int idx = 0; idx < values.size(); ++idx)
	{
		meshes[idx] = mCubes.triangulateFieldGPU(_ssbo, _numDivs, values[idx], transformationMatrix);
	}

	return meshes;
}

void RegularGrid::updateSSBO()
{
	ComputeShader::updateReadBuffer(_ssbo, _grid.data(), _grid.size(), GL_DYNAMIC_DRAW);
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
	for (unsigned int x = 0; x < _numDivs.x; ++x)
		for (unsigned int y = 0; y < _numDivs.y; ++y)
			for (unsigned int z = 0; z < _numDivs.z; ++z)
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
	_grid = std::vector<CellGrid>(_numDivs.x * _numDivs.y * _numDivs.z);
	std::fill(_grid.begin(), _grid.end(), CellGrid());

	_ssbo = ComputeShader::setReadBuffer(_grid.data(), _grid.size(), GL_DYNAMIC_DRAW);
}

size_t RegularGrid::countValues(std::unordered_set<uint16_t>& values)
{
	unsigned index;

	for (unsigned int x = 0; x < _numDivs.x; ++x)
		for (unsigned int y = 0; y < _numDivs.y; ++y)
			for (unsigned int z = 0; z < _numDivs.z; ++z)
			{
				index = this->getPositionIndex(x, y, z);
				if (_grid[index]._value > VOXEL_FREE)
					values.insert(_grid[index]._value);
			}

	return values.size();
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

unsigned RegularGrid::getPositionIndex(int x, int y, int z, const uvec3& numDivs)
{
	return x * numDivs.y * numDivs.z + y * numDivs.z + z;
}
