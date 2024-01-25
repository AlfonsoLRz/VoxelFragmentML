#include "stdafx.h"
#include "FloodFracturer.h"

#include "Graphics/Core/ShaderList.h"

namespace fracturer {

	const std::vector<glm::ivec4> FloodFracturer::VON_NEUMANN = {
		glm::ivec4(1, 0, 0, 0), glm::ivec4(-1, 0, 0, 0), glm::ivec4(0, 1, 0, 0),
		glm::ivec4(0, -1, 0, 0), glm::ivec4(0, 0, 1, 0), glm::ivec4(0, 0, -1, 0)
	};

	const std::vector<glm::ivec4> FloodFracturer::MOORE = {
		glm::ivec4(-1, -1, -1, 0), glm::ivec4(1, 1, 1, 0),
		glm::ivec4(-1, -1, 0, 0), glm::ivec4(1, 1, 0, 0),
		glm::ivec4(-1, -1, 1, 0), glm::ivec4(1, 1, -1, 0),
		glm::ivec4(0, -1, -1, 0), glm::ivec4(0, 1, 1, 0),
		glm::ivec4(0, -1, 0, 0), glm::ivec4(0, 1, 0, 0),
		glm::ivec4(0, -1, 1, 0), glm::ivec4(0, 1, -1, 0),
		glm::ivec4(1, -1, -1, 0), glm::ivec4(-1, 1, 1, 0),
		glm::ivec4(1, -1, 0, 0), glm::ivec4(-1, 1, 0, 0),
		glm::ivec4(1, -1, 1, 0), glm::ivec4(-1, 1, -1, 0),
		glm::ivec4(-1, 0, -1, 0), glm::ivec4(1, 0, 1, 0),
		glm::ivec4(-1, 0, 0, 0), glm::ivec4(1, 0, 0, 0),
		glm::ivec4(-1, 0, 1, 0), glm::ivec4(1, 0, -1, 0),
		glm::ivec4(0, 0, -1, 0), glm::ivec4(0, 0, 1, 0)
	};

	FloodFracturer::FloodFracturer() : _dfunc(MANHATTAN_DISTANCE)
	{
		_fractureShader = ShaderList::getInstance()->getComputeShader(RendEnum::FLOOD_FRACTURER);
		_disjointSetShader = ShaderList::getInstance()->getComputeShader(RendEnum::DISJOINT_SET);
		_disjointSetStackShader = ShaderList::getInstance()->getComputeShader(RendEnum::DISJOINT_SET_STACK);
		_unmaskShader = ShaderList::getInstance()->getComputeShader(RendEnum::UNDO_MASK_SHADER);

		_disjointCounterSSBO = std::numeric_limits<GLuint>::max();
		_disjointSetSSBO = std::numeric_limits<GLuint>::max();
		_disjointSetZero = nullptr;

		_numCells = 0;
		_neighborSSBO = std::numeric_limits<GLuint>::max();
		_stackSizeSSBO = std::numeric_limits<GLuint>::max();
		_stack1SSBO = std::numeric_limits<GLuint>::max();
		_stack2SSBO = std::numeric_limits<GLuint>::max();
	}

	void FloodFracturer::init(FractureParameters* fractParameters)
	{
		_disjointCounterSSBO = ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);
		_disjointSetSSBO = ComputeShader::setWriteBuffer(GLuint(), std::pow(2, Seeder::VOXEL_ID_POSITION), GL_DYNAMIC_DRAW);
		_disjointSetZero = (uint16_t*)malloc(std::pow(2, Seeder::VOXEL_ID_POSITION) * sizeof(GLuint));
		std::iota(_disjointSetZero, _disjointSetZero + glm::uint(std::pow(2, Seeder::VOXEL_ID_POSITION)), std::numeric_limits<GLuint>::max());

		_numCells = fractParameters->_voxelizationSize.x * fractParameters->_voxelizationSize.y * fractParameters->_voxelizationSize.z;
		_stack1SSBO = ComputeShader::setWriteBuffer(GLuint(), _numCells, GL_DYNAMIC_DRAW);
		_stack2SSBO = ComputeShader::setWriteBuffer(GLuint(), _numCells, GL_DYNAMIC_DRAW);
		_neighborSSBO = ComputeShader::setReadBuffer(fractParameters->_neighbourhoodType == FractureParameters::VON_NEUMANN ? VON_NEUMANN : MOORE, GL_STATIC_DRAW);
		_stackSizeSSBO = ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);
	}

	void FloodFracturer::destroy()
	{
		_numCells = 0;

		if (_disjointSetSSBO != std::numeric_limits<GLuint>::max())
		{
			ComputeShader::deleteBuffer(_disjointCounterSSBO);
			ComputeShader::deleteBuffer(_disjointSetSSBO);
			_disjointSetSSBO = std::numeric_limits<GLuint>::max();
			free(_disjointSetZero);
		}

		if (_stack1SSBO != std::numeric_limits<GLuint>::max())
		{
			ComputeShader::deleteBuffer(_stack1SSBO);
			_stack1SSBO = std::numeric_limits<GLuint>::max();
		}

		if (_stack2SSBO != std::numeric_limits<GLuint>::max())
		{
			ComputeShader::deleteBuffer(_stack2SSBO);
			_stack2SSBO = std::numeric_limits<GLuint>::max();
		}

		if (_neighborSSBO != std::numeric_limits<GLuint>::max())
		{
			ComputeShader::deleteBuffer(_neighborSSBO);
			_neighborSSBO = std::numeric_limits<GLuint>::max();
		}

		if (_stackSizeSSBO != std::numeric_limits<GLuint>::max())
		{
			ComputeShader::deleteBuffer(_stackSizeSSBO);
			_stackSizeSSBO = std::numeric_limits<GLuint>::max();
		}
	}

	void FloodFracturer::build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters) {
		grid.homogenize();

		// Set seeds
		for (auto& seed : seeds)
			grid.set(seed.x, seed.y, seed.z, seed.w);

		grid.updateSSBO();

		// Input data
		uvec3 numDivs = grid.getNumSubdivisions();
		unsigned iteration = 0;
		unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
		unsigned nullCount = 0;
		unsigned stackSize = seeds.size();
		RegularGrid::CellGrid* gridData = grid.data();
		unsigned numNeigh = _dfunc == 1 ? GLuint(VON_NEUMANN.size()) : GLuint(MOORE.size());

		if (numCells > _numCells)
		{
			this->destroy();
			this->init(fractParameters);
		}
		else
		{
			ComputeShader::updateReadBufferSubset(_neighborSSBO, _dfunc == 1 ? VON_NEUMANN.data() : MOORE.data(), 0, _dfunc == 1 ? VON_NEUMANN.size() : MOORE.size());
		}

		// Load seeds as a subset
		std::vector<GLuint> seedsInt;
		for (auto& seed : seeds) seedsInt.push_back(RegularGrid::getPositionIndex(seed.x, seed.y, seed.z, numDivs));

		ComputeShader::updateReadBufferSubset(_stack1SSBO, seedsInt.data(), 0, seeds.size());

		glm::uint numDisjointVoxels = stackSize;
		while (numDisjointVoxels != 0)
		{
			std::cout << "Iteration " << iteration << " - " << numDisjointVoxels << " voxels to process" << std::endl;

			_fractureShader->use();
			_fractureShader->setUniform("gridDims", numDivs);
			_fractureShader->setUniform("numNeighbors", numNeigh);

			while (stackSize > 0)
			{
				unsigned numGroups = ComputeShader::getNumGroups(stackSize * numNeigh);
				ComputeShader::updateReadBufferSubset(_stackSizeSSBO, &nullCount, 0, 1);

				_fractureShader->bindBuffers(std::vector<GLuint>{ grid.ssbo(), _stack1SSBO, _stack2SSBO, _stackSizeSSBO, _neighborSSBO });
				_fractureShader->setUniform("stackSize", stackSize);
				_fractureShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

				stackSize = *ComputeShader::readData(_stackSizeSSBO, GLuint());

				//  Swap buffers
				std::swap(_stack1SSBO, _stack2SSBO);

				++iteration;
			}

			// Now we have to remove isolated regions
			ComputeShader::updateReadBufferSubset(_disjointSetSSBO, _disjointSetZero, 0, std::pow(2, Seeder::VOXEL_ID_POSITION));
			ComputeShader::updateReadBufferSubset(_disjointCounterSSBO, &nullCount, 0, 1);
			ComputeShader::updateReadBufferSubset(_stackSizeSSBO, &nullCount, 0, 1);

			_disjointSetShader->use();
			_disjointSetShader->bindBuffers(std::vector<GLuint>{ grid.ssbo(), _disjointSetSSBO });
			_disjointSetShader->setUniform("gridDims", numDivs);
			_disjointSetShader->execute(ComputeShader::getNumGroups(numCells), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

			_disjointSetStackShader->use();
			_disjointSetStackShader->bindBuffers(std::vector<GLuint>{ grid.ssbo(), _disjointSetSSBO, _stack1SSBO, _stackSizeSSBO, _disjointCounterSSBO });
			_disjointSetStackShader->setUniform("gridDims", numDivs);
			_disjointSetStackShader->execute(ComputeShader::getNumGroups(numCells), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

			stackSize = *ComputeShader::readData(_stackSizeSSBO, GLuint());
			numDisjointVoxels = *ComputeShader::readData(_disjointCounterSSBO, GLuint());
		}

		// Remove mask
		_unmaskShader->use();
		_unmaskShader->bindBuffers(std::vector<GLuint>{ grid.ssbo() });
		_unmaskShader->setUniform("numCells", numCells);
		_unmaskShader->setUniform("position", glm::uint(Seeder::VOXEL_ID_POSITION));
		_unmaskShader->setSubroutineUniform(GL_COMPUTE_SHADER, "unmaskUniform", "unmaskRightMost");
		_unmaskShader->applyActiveSubroutines();
		_unmaskShader->execute(ComputeShader::getNumGroups(numCells), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		//RegularGrid::CellGrid* resultPointer = ComputeShader::readData(grid.ssbo(), RegularGrid::CellGrid());
		//std::vector<RegularGrid::CellGrid> resultBuffer = std::vector<RegularGrid::CellGrid>(resultPointer, resultPointer + numCells);
		//grid.swap(resultBuffer);
	}

	void FloodFracturer::prepareSSBOs(FractureParameters* fractParameters)
	{
		//this->destroy();
		this->init(fractParameters);
	}

	bool FloodFracturer::setDistanceFunction(DistanceFunction dfunc)
	{
		// EUCLIDEAN_DISTANCE is forbid
		//if (dfunc == EUCLIDEAN_DISTANCE)
		//	//throw std::invalid_argument("Flood method is not compatible with euclidean metric. Try with naive method.");
		//	return false;

		_dfunc = dfunc;
		return true;
	}
}