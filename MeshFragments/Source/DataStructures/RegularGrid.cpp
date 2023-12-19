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
	ComputeShader::deleteBuffer(_countSSBO);
	ComputeShader::deleteBuffer(_ssbo);
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

	uint32_t maskSize = convolutionSize * convolutionSize * convolutionSize, convolutionCenter = std::floor(convolutionSize / 2.0f);
	float activations = 0;
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

	activations /= maskSize;

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
		this->detectBoundaries(1);

		_erodeShader->bindBuffers(std::vector<GLuint>{ _ssbo, maskSSBO, noiseSSBO });
		_erodeShader->use();
		_erodeShader->setUniform("numActivationsFloat", activations);
		_erodeShader->setUniform("gridDims", numDivs);
		_erodeShader->setUniform("maskSize", convolutionSize);
		_erodeShader->setUniform("maskSize2", unsigned(std::floor(convolutionSize / 2.0f)));
		_erodeShader->setUniform("numCells", numCells);
		_erodeShader->setUniform("noiseBufferSize", static_cast<unsigned>(noiseBuffer.size()));
		_erodeShader->setUniform("erosionProbability", erosionProbability);
		_erodeShader->setUniform("erosionThreshold", erosionThreshold);
		_erodeShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
	}

	this->removeIsolatedRegions();

	CellGrid* gridData = ComputeShader::readData(_ssbo, CellGrid());
	std::copy(gridData, gridData + numCells, _grid.begin());

	ComputeShader::deleteBuffers(std::vector<GLuint>{ maskSSBO, noiseSSBO });
}

void RegularGrid::exportGrid(bool squared, const std::string& filename)
{
	vox::VoxWriter vox;

	// Check if squared is required
	uvec3 end = _numDivs, start = uvec3(0);
	if (squared)
	{
		ivec3 currentPosition;
		end.x = end.y = end.z = glm::max(end.x, glm::max(end.y, end.z));
		start = (end - _numDivs) / uvec3(2);

		// Reset first
		for (int x = 0; x < end.x; ++x)
		{
			for (int y = 0; y < end.y; ++y)
			{
				for (int z = 0; z < end.z; ++z)
				{
					currentPosition = ivec3(x, y, z) - ivec3(start);

					if (glm::all(glm::greaterThanEqual(currentPosition, ivec3(0))) && glm::all(glm::lessThan(currentPosition, ivec3(_numDivs))))
						vox.AddVoxel(x, z, y, this->at(currentPosition.x, currentPosition.y, currentPosition.z));
					else
						vox.AddVoxel(x, z, y, VOXEL_EMPTY);
				}
			}
		}
	}
	else
	{
		for (int x = 0; x < _numDivs.x; ++x)
		{
			unsigned positionIndex;

			for (int y = 0; y < _numDivs.y; ++y)
			{
				for (int z = 0; z < _numDivs.z; ++z)
				{
					positionIndex = this->getPositionIndex(x, y, z);
					if (_grid[positionIndex]._value >= VOXEL_FREE)
						vox.AddVoxel(x + start.x, z + start.z, y + start.y, _grid[positionIndex]._value - VOXEL_EMPTY);
				}
			}
		}
	}

	// If path is empty then save in a file with random numbering
	std::string filePath = filename;
	if (filePath.empty())
		filePath = "Output/grid" + std::to_string(rand()) + ".vox";

	vox.SaveToFile(filePath);
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
				if (_grid[this->getPositionIndex(x, y, z)]._value >= VOXEL_FREE)
				{
					min = _aabb.min() + _cellSize * vec3(x, y, z);
					max = min + _cellSize;

					aabb.push_back(AABB(min, max));
				}
			}
		}
	}
}

uvec3 RegularGrid::getClosestEntryVoxel(const Model3D::RayGPUData& ray)
{
	return this->rayTraversalAmanatidesWoo(ray);
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

		ComputeShader::deleteBuffer(faceSSBO);
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

	ComputeShader::deleteBuffers(std::vector<GLuint> { countSSBO, vertexSSBO, gridSSBO, boundarySSBO, noiseSSBO });
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

	ComputeShader::deleteBuffers(std::vector<GLuint> { vertexSSBO, gridSSBO, clusterSSBO });
}

void RegularGrid::resetFilling()
{
	size_t numCells = _numDivs.x * _numDivs.y * _numDivs.z;
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

bool RegularGrid::isBoundary(int x, int y, int z, int neighbourhoodSize) const
{
	if (neighbourhoodSize % 2 == 0)
		++neighbourhoodSize;

	ivec3 min = glm::clamp(ivec3(x, y, z) - ivec3(neighbourhoodSize), ivec3(0), ivec3(_numDivs) - ivec3(1));
	ivec3 max = glm::clamp(ivec3(x, y, z) + ivec3(neighbourhoodSize), ivec3(0), ivec3(_numDivs) - ivec3(1));
	bool isBoundary = false;

	for (int x = min.x; x <= max.x && !isBoundary; ++x)
		for (int y = min.y; y <= max.y && !isBoundary; ++y)
			for (int z = min.z; z <= max.z && !isBoundary; ++z)
				if (this->at(x, y, z) == VOXEL_EMPTY)
					isBoundary = true;

	return isBoundary;
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
	unsigned numCells = _numDivs.x * _numDivs.y * _numDivs.z;
	//_grid = std::vector<CellGrid>(_numDivs.x * _numDivs.y * _numDivs.z);
	std::fill(_grid.begin(), _grid.begin() + numCells, CellGrid());
	std::fill(_voxelOpenGL.begin(), _voxelOpenGL.begin() + numCells, 0);

	ComputeShader::updateReadBufferSubset(_ssbo, _grid.data(), 0, numCells);
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
	_removeIsolatedRegionsShader = ShaderList::getInstance()->getComputeShader(RendEnum::REMOVE_ISOLATED_REGIONS_GRID);
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

bool RegularGrid::rayBoxIntersection(const Model3D::RayGPUData& ray, float& tMin, float& tMax, float t0, float t1)
{
	vec3 rayStart = ray._origin, rayDirection = ray._direction;
	vec3 minBound = _aabb.min(), maxBound = _aabb.max();
	float tYMin, tYMax, tZMin, tZMax;

	const float x_inv_dir = 1 / rayDirection.x;
	if (x_inv_dir >= 0) {
		tMin = (minBound.x - rayStart.x) * x_inv_dir;
		tMax = (maxBound.x - rayStart.x) * x_inv_dir;
	}
	else {
		tMin = (maxBound.x - rayStart.x) * x_inv_dir;
		tMax = (minBound.x - rayStart.x) * x_inv_dir;
	}

	const float y_inv_dir = 1 / rayDirection.y;
	if (y_inv_dir >= 0) {
		tYMin = (minBound.y - rayStart.y) * y_inv_dir;
		tYMax = (maxBound.y - rayStart.y) * y_inv_dir;
	}
	else {
		tYMin = (maxBound.y - rayStart.y) * y_inv_dir;
		tYMax = (minBound.y - rayStart.y) * y_inv_dir;
	}

	if (tMin > tYMax || tYMin > tMax) return false;
	if (tYMin > tMin) tMin = tYMin;
	if (tYMax < tMax) tMax = tYMax;

	const float z_inv_dir = 1 / rayDirection.z;
	if (z_inv_dir >= 0) {
		tZMin = (minBound.z - rayStart.z) * z_inv_dir;
		tZMax = (maxBound.z - rayStart.z) * z_inv_dir;
	}
	else {
		tZMin = (maxBound.z - rayStart.z) * z_inv_dir;
		tZMax = (minBound.z - rayStart.z) * z_inv_dir;
	}

	if (tMin > tZMax || tZMin > tMax) return false;
	if (tZMin > tMin) tMin = tZMin;
	if (tZMax < tMax) tMax = tZMax;

	return (tMin < t1 && tMax > t0);
}

uvec3 RegularGrid::rayTraversalAmanatidesWoo(const Model3D::RayGPUData& ray)
{
	float t0 = .0f, t1 = FLT_MAX, tMin = .0f, tMax;
	const bool intersects = this->rayBoxIntersection(ray, tMin, tMax, t0, t1);
	if (!intersects) return uvec3(std::numeric_limits<glm::uint>::max());

	tMin = glm::max(tMin, t0);
	tMax = glm::max(tMax, t1);

	float tDeltaX, tMaxX, tDeltaY, tMaxY, tDeltaZ, tMaxZ;
	vec3 rayDirection = ray._direction;
	vec3 rayStart = ray._origin + rayDirection * tMin, rayEnd = ray._origin + rayDirection * tMax;
	vec3 minBound = _aabb.min();
	uvec3 current_index = 
		uvec3(
			glm::max(1.0f, std::ceil((rayStart.x - minBound.x) / _cellSize.x)), 
			glm::max(1.0f, std::ceil((rayStart.y - minBound.y) / _cellSize.y)),
			glm::max(1.0f, std::ceil((rayStart.z - minBound.z) / _cellSize.z)));
	ivec3 step;

	if (rayDirection.x > 0.0)
	{
		step.x = 1;
		tDeltaX = _cellSize.x / rayDirection.x;
		tMaxX = tMin + (minBound.x + current_index.x * _cellSize.x - rayStart.x) / rayDirection.x;
	}
	else if (rayDirection.x < 0.0)
	{
		step.x = -1;
		tDeltaX = _cellSize.x / -rayDirection.x;
		const size_t previous_X_index = current_index.x - 1;
		tMaxX = tMin + (minBound.x + previous_X_index * _cellSize.x - rayStart.x) / rayDirection.x;
	}
	else
	{
		step.x = 0;
		tDeltaX = tMax;
		tMaxX = tMax;
	}

	if (rayDirection.y > 0.0)
	{
		step.y = 1;
		tDeltaY = _cellSize.y / rayDirection.y;
		tMaxY = tMin + (minBound.y + current_index.y * _cellSize.y - rayStart.y) / rayDirection.y;
	}
	else if (rayDirection.y < 0.0)
	{
		step.y = -1;
		tDeltaY = _cellSize.y / -rayDirection.y;
		const size_t previous_Y_index = current_index.y - 1;
		tMaxY = tMin + (minBound.y + previous_Y_index * _cellSize.y - rayStart.y) / rayDirection.y;
	}
	else
	{
		step.y = 0;
		tDeltaY = tMax;
		tMaxY = tMax;
	}

	if (rayDirection.z > 0.0)
	{
		step.z = 1;
		tDeltaZ = _cellSize.z / rayDirection.z;
		tMaxZ = tMin + (minBound.z + current_index.z * _cellSize.z - rayStart.z) / rayDirection.z;
	}
	else if (rayDirection.z < 0.0)
	{
		step.z = -1;
		tDeltaZ = _cellSize.z / -rayDirection.z;
		const size_t previous_Z_index = current_index.z - 1;
		tMaxZ = tMin + (minBound.z + previous_Z_index * _cellSize.z - rayStart.z) / rayDirection.z;
	}
	else
	{
		step.z = 0;
		tDeltaZ = tMax;
		tMaxZ = tMax;
	}

	while (current_index.x <= _numDivs.x && current_index.y <= _numDivs.y && current_index.z <= _numDivs.z
		&& current_index.x > 0 && current_index.y > 0 && current_index.z > 0)
	{
		if (this->at(current_index.x - 1, current_index.y - 1, current_index.z - 1) != VOXEL_EMPTY)
			return current_index;

		if (tMaxX < tMaxY && tMaxX < tMaxZ) {
			// X-axis traversal
			current_index.x += step.x;
			tMaxX += tDeltaX;
		}
		else if (tMaxY < tMaxZ) {
			// Y-axis traversal
			current_index.y += step.y;
			tMaxY += tDeltaY;
		}
		else {
			// Z-axis traversal
			current_index.z += step.z;
			tMaxZ += tDeltaZ;
		}
	}

	return uvec3(std::numeric_limits<glm::uint>::max());
}

void RegularGrid::removeIsolatedRegions()
{
	unsigned numCells = _numDivs.x * _numDivs.y * _numDivs.z;

	_removeIsolatedRegionsShader->bindBuffers(std::vector<GLuint>{ _ssbo });
	_removeIsolatedRegionsShader->use();
	_removeIsolatedRegionsShader->setUniform("gridDims", this->getNumSubdivisions());
	_removeIsolatedRegionsShader->setUniform("numCells", numCells);
	_removeIsolatedRegionsShader->execute(ComputeShader::getNumGroups(numCells), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
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
