#include "stdafx.h"
#include "NaiveFracturer.h"

#include "Graphics/Core/ShaderList.h"
#include "Utilities/ChronoUtilities.h"

namespace fracturer {
	// [Public methods]
	
    NaiveFracturer::NaiveFracturer() : _dfunc(EUCLIDEAN_DISTANCE), _spaceTexture(0)
    {
        _distanceFunctionMap[FractureParameters::DistanceFunction::EUCLIDEAN] = [](const ivec3& pos1, const ivec4& pos2) -> float {
            float x = pos1.x - pos2.x, y = pos1.y - pos2.y, z = pos1.z - pos2.z;
            return sqrt(x * x + y * y + z * z);
        };

        _distanceFunctionMap[FractureParameters::DistanceFunction::MANHATTAN] = [](const ivec3& pos1, const ivec4& pos2) -> float {
            return glm::abs(pos1.x - pos2.x) + glm::abs(pos1.y - pos2.y) + glm::abs(pos1.z - pos2.z);
        };

        _distanceFunctionMap[FractureParameters::DistanceFunction::CHEBYSHEV] = [](const ivec3& pos1, const ivec4& pos2) -> float {
            return std::max(glm::abs(pos1.x - pos2.x), std::max(glm::abs(pos1.y - pos2.y), glm::abs(pos1.z - pos2.z)));
        };
    }

    void NaiveFracturer::buildCPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters)
    {
        uvec3 numDivs = grid.getNumSubdivisions();
        unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
        uint16_t* gridData = grid.data();
        DistFunction distanceFunction = _distanceFunctionMap[fractParameters->_distanceFunction];

    	// Iteration data
        ivec3 cellPoint;
        float distance, minDistance;
        GLuint seed, cellIndex;

    	for (int x = 0; x < numDivs.x; ++x)
    	{
            for (int y = 0; y < numDivs.y; ++y)
            {
                for (int z = 0; z < numDivs.z; ++z)
                {
                    cellPoint = ivec3(x, y, z);
                    cellIndex = RegularGrid::getPositionIndex(x, y, z, numDivs);
                    minDistance = FLT_MAX;

                    if (gridData[cellIndex] == VOXEL_EMPTY) continue;

                    for (int seedIdx = 0; seedIdx < seeds.size(); ++seedIdx)
                    {
                        distance = distanceFunction(cellPoint, seeds[seedIdx]);

                        if (distance < minDistance)
                        {
                            minDistance = distance;
                            gridData[cellIndex] = seeds[seedIdx].w;
                        }
                    }
                }
            }
    	}

        if (fractParameters->_removeIsolatedRegions)
        {
            this->removeIsolatedRegionsCPU(grid, seeds);
        }
    }


    void NaiveFracturer::buildGPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters)
    {
        ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::NAIVE_FRACTURER);
        const char* distanceUniform[FractureParameters::DISTANCE_FUNCTIONS] = { "euclideanDistance", "manhattanDistance", "chebyshevDistance" };

        // Input data
        uvec3 numDivs = grid.getNumSubdivisions();
        unsigned numThreads = numDivs.x * numDivs.y * numDivs.z;
        unsigned numGroups = ComputeShader::getNumGroups(numThreads);
        uint16_t* gridData = grid.data();

        const GLuint seedSSBO = ComputeShader::setReadBuffer(seeds, GL_STATIC_DRAW);
        const GLuint gridSSBO = ComputeShader::setReadBuffer(&gridData[0], numThreads, GL_DYNAMIC_DRAW);

        shader->bindBuffers(std::vector<GLuint>{ seedSSBO, gridSSBO });
        shader->use();
        shader->setUniform("gridDims", numDivs);
        shader->setUniform("numSeeds", GLuint(seeds.size()));
        shader->setSubroutineUniform(GL_COMPUTE_SHADER, "distanceUniform", distanceUniform[_dfunc]);
        shader->applyActiveSubroutines();
        shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

        if (fractParameters->_removeIsolatedRegions)
        {
            this->removeIsolatedRegionsGPU(gridSSBO, grid, seeds);
        }
        else
        {
            uint16_t* resultPointer = ComputeShader::readData(gridSSBO, uint16_t());
            std::vector<uint16_t> resultBuffer = std::vector<uint16_t>(resultPointer, resultPointer + numThreads);

            grid.swap(resultBuffer);
        }

        glDeleteBuffers(1, &seedSSBO);
        glDeleteBuffers(1, &gridSSBO);
    }

    void NaiveFracturer::removeIsolatedRegionsCPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds)
    {
        uvec3 numDivs = grid.getNumSubdivisions();
        unsigned numCells = numDivs.x * numDivs.y * numDivs.z, cellIndex;
        std::list<glm::uvec4> front(seeds.begin(), seeds.end());                    // Linked list where to push/pop voxels
        std::vector<uint16_t> newGrid(numCells);
        std::fill(newGrid.begin(), newGrid.end(), VOXEL_EMPTY);
        uint16_t* gridData = grid.data();

        for (glm::uvec4 seed : seeds)
        {
            newGrid[RegularGrid::getPositionIndex(seed.x, seed.y, seed.z, numDivs)] = seed.w;
        }

        // Iterate while voxel list is not empty
        while (!front.empty()) {
            glm::uvec4 v = front.front();

            // Expand front
			#define expand(dx, dy, dz)\
            cellIndex = RegularGrid::getPositionIndex(v.x + (dx), v.y + (dy), v.z + (dz), numDivs);\
            if (gridData[cellIndex] == v.w && newGrid[cellIndex] == VOXEL_EMPTY) {\
                front.push_back(glm::uvec4(v.x + (dx), v.y + (dy), v.z + (dz), v.w));\
                newGrid[cellIndex] = v.w;\
            }

            // Remove from list
            front.pop_front();

            if (v.x < numDivs.x - 1) { expand(+1, 0, 0); }
            if (v.x > 0) { expand(-1, 0, 0); }
            if (v.y < numDivs.y - 1) { expand(0, +1, 0); }
            if (v.y > 0) { expand(0, -1, 0); }
            if (v.z < numDivs.z - 1) { expand(0, 0, +1); }
            if (v.z > 0) { expand(0, 0, -1); }
        }

        // Move operator
        grid.swap(newGrid);
    }

    void NaiveFracturer::removeIsolatedRegionsGPU(const GLuint gridSSBO, RegularGrid& grid, const std::vector<glm::uvec4>& seeds)
    {
		// Set seeds
		for (auto& seed : seeds)
	       grid.set(seed.x, seed.y, seed.z, seed.w);

		ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::REMOVE_ISOLATED_REGIONS);

		// Define neighbors
		std::vector<ivec4> offset{ ivec4(+1, 0, 0, 0), ivec4(-1, 0, 0, 0), ivec4(0, +1, 0, 0), ivec4(0, -1, 0, 0), ivec4(0, 0, +1, 0), ivec4(0, 0, -1, 0) };

		// Input data
		uvec3 numDivs       = grid.getNumSubdivisions();
		unsigned numCells   = numDivs.x * numDivs.y * numDivs.z;
		unsigned nullCount  = 0;
		unsigned stackSize  = seeds.size();
		uint16_t* gridData  = ComputeShader::readData(gridSSBO, uint16_t());;
		unsigned numNeigh   = offset.size();

		// New regular grid
        std::vector<uint16_t> newGrid(gridData, gridData + numCells);
		for (glm::uvec4 seed : seeds)
		{
            newGrid[RegularGrid::getPositionIndex(seed.x, seed.y, seed.z, numDivs)] = seed.w;
		}

		GLuint stack1SSBO = ComputeShader::setWriteBuffer(GLuint(), numCells, GL_DYNAMIC_DRAW);
		GLuint stack2SSBO = ComputeShader::setWriteBuffer(GLuint(), numCells, GL_DYNAMIC_DRAW);
		const GLuint newGridSSBO = ComputeShader::setReadBuffer(&newGrid[0], numCells, GL_DYNAMIC_DRAW);
		const GLuint neighborSSBO = ComputeShader::setReadBuffer(offset, GL_STATIC_DRAW);
		const GLuint stackSizeSSBO = ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);

		// Load seeds as a subset
		std::vector<GLuint> seedsInt;
		for (auto& seed : seeds) seedsInt.push_back(RegularGrid::getPositionIndex(seed.x, seed.y, seed.z, numDivs));

		ComputeShader::updateReadBufferSubset(stack1SSBO, seedsInt.data(), 0, seeds.size());

		shader->use();
		shader->setUniform("gridDims", numDivs);
		shader->setUniform("numNeighbors", numNeigh);

		while (stackSize > 0)
		{
			unsigned numGroups = ComputeShader::getNumGroups(stackSize * numNeigh);
			ComputeShader::updateReadBuffer(stackSizeSSBO, &nullCount, 1, GL_DYNAMIC_DRAW);

			shader->bindBuffers(std::vector<GLuint>{ gridSSBO, newGridSSBO, stack1SSBO, stack2SSBO, stackSizeSSBO, neighborSSBO });
			shader->setUniform("stackSize", stackSize);
			shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

			stackSize = *ComputeShader::readData(stackSizeSSBO, GLuint());

			// Swap buffers
			std::swap(stack1SSBO, stack2SSBO);
		}

		uint16_t* resultPointer = ComputeShader::readData(newGridSSBO, uint16_t());
		grid.swap(std::vector<uint16_t>(resultPointer, resultPointer + numCells));

		GLuint deleteBuffers[] = { stack1SSBO, stack2SSBO, newGridSSBO, neighborSSBO, stackSize };
		glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);
    }

    void NaiveFracturer::build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters)
	{
        if (fractParameters->_launchGPU)
        {
            this->buildGPU(grid, seeds, fractParameters);
        }
        else
        {
            this->buildCPU(grid, seeds, fractParameters);
        }
    }

    void NaiveFracturer::setDistanceFunction(DistanceFunction dfunc)
	{
        _dfunc = dfunc;
    }
}
