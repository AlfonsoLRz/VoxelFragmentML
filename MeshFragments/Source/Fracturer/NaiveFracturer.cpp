#include "stdafx.h"
#include "NaiveFracturer.h"

#include "Graphics/Core/ShaderList.h"

namespace fracturer {
	// [Public methods]
	
    NaiveFracturer::NaiveFracturer() : _dfunc(EUCLIDEAN_DISTANCE), _spaceTexture(0)
    {
    }

    void NaiveFracturer::removeIsolatedRegions(RegularGrid& grid, const std::vector<glm::uvec4>& seeds)
	{
        // Final 3D image
        RegularGrid newGrid (grid.getNumSubdivisions());

        // Linked list where to push/pop voxels
        std::list<glm::uvec4> front(seeds.begin(), seeds.end());

        for (glm::uvec4 seed : seeds) 
        {
            newGrid.set(seed.x, seed.y, seed.z, seed.w);
        }

        // Iterate while voxel list is not empty
        while (!front.empty()) {
            // Get voxel
            glm::uvec4 v = front.front();

            // Expand front
            #define expand(dx, dy, dz)\
                if (grid.at(v.x + (dx), v.y + (dy), v.z + (dz)) ==\
                    v.w && newGrid.at(v.x + (dx), v.y +(dy), v.z + (dz)) == VOXEL_EMPTY) {\
                    front.push_back(glm::uvec4(v.x + (dx), v.y + (dy), v.z + (dz), v.w));\
                    newGrid.set(v.x + (dx), v.y + (dy), v.z + (dz), v.w);\
                }

            // Remove from list
            front.pop_front();

            expand(+1, 0, 0);
            expand(-1, 0, 0);
            expand(0, +1, 0);
            expand(0, -1, 0);
            expand(0, 0, +1);
            expand(0, 0, -1);
        }

        // Move operator
        grid = std::move(newGrid);

        // Set seeds
     //   for (auto& seed : seeds)
     //       grid.set(seed.x, seed.y, seed.z, seed.w);

     //   ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::REMOVE_ISOLATED_REGIONS);

    	//// Define neighbors
    	//std::vector<ivec4> offset { ivec4(+1, 0, 0, 0), ivec4(-1, 0, 0, 0), ivec4(0, +1, 0, 0), ivec4(0, -1, 0, 0), ivec4(0, 0, +1, 0), ivec4(0, 0, -1, 0) }

     //   // Input data
     //   uvec3 numDivs       = grid.getNumSubdivisions();
     //   unsigned numCells   = numDivs.x * numDivs.y * numDivs.z;
     //   unsigned nullCount  = 0;
     //   unsigned stackSize  = seeds.size();
     //   uint16_t* gridData  = grid.data();
     //   unsigned numNeigh   = 6;

     //   GLuint stack1SSBO = ComputeShader::setWriteBuffer(GLuint(), numCells, GL_DYNAMIC_DRAW);
     //   GLuint stack2SSBO = ComputeShader::setWriteBuffer(GLuint(), numCells, GL_DYNAMIC_DRAW);
     //   const GLuint gridSSBO = ComputeShader::setReadBuffer(&gridData[0], numCells, GL_DYNAMIC_DRAW);
     //   const GLuint gridSSBO = ComputeShader::setWriteBuffer(&gridData[0], numCells, GL_DYNAMIC_DRAW);
     //   const GLuint neighborSSBO = ComputeShader::setReadBuffer(offset, GL_STATIC_DRAW);
     //   const GLuint stackSizeSSBO = ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);

     //   // Load seeds as a subset
     //   std::vector<GLuint> seedsInt;
     //   for (auto& seed : seeds) seedsInt.push_back(RegularGrid::getPositionIndex(seed.x, seed.y, seed.z, numDivs));

     //   ComputeShader::updateReadBufferSubset(stack1SSBO, seedsInt.data(), 0, seeds.size());

     //   shader->use();
     //   shader->setUniform("gridDims", numDivs);
     //   shader->setUniform("numNeighbors", numNeigh);

     //   while (stackSize > 0)
     //   {
     //       unsigned numGroups = ComputeShader::getNumGroups(stackSize * numNeigh);
     //       ComputeShader::updateReadBuffer(stackSizeSSBO, &nullCount, 1, GL_DYNAMIC_DRAW);

     //       shader->bindBuffers(std::vector<GLuint>{ gridSSBO, newGridSSBO, stack1SSBO, stack2SSBO, stackSizeSSBO, neighborSSBO });
     //       shader->setUniform("stackSize", stackSize);
     //       shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

     //       stackSize = *ComputeShader::readData(stackSizeSSBO, GLuint());

     //       //  Swap buffers
     //       std::swap(stack1SSBO, stack2SSBO);
     //   }

     //   uint16_t* resultPointer = ComputeShader::readData(gridSSBO, uint16_t());
     //   resultBuffer = std::vector<uint16_t>(resultPointer, resultPointer + numCells);

     //   GLuint deleteBuffers[] = { stack1SSBO, stack2SSBO, gridSSBO, neighborSSBO, stackSize };
     //   glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);
    }

    void NaiveFracturer::init() {
    }

    void NaiveFracturer::destroy()
	{
    }

    void NaiveFracturer::build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, std::vector<uint16_t>& resultBuffer, FractureParameters* fractParameters)
	{
        ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::NAIVE_FRACTURER);
        const char* distanceUniform[FractureParameters::DISTANCE_FUNCTIONS] = { "euclideanDistance", "manhattanDistance", "chebyshevDistance" };

        // Input data
        uvec3 numDivs       = grid.getNumSubdivisions();
        unsigned numThreads = numDivs.x * numDivs.y * numDivs.z;
        unsigned numGroups  = ComputeShader::getNumGroups(numThreads);
        uint16_t* gridData  = grid.data();

        const GLuint seedSSBO   = ComputeShader::setReadBuffer(seeds, GL_STATIC_DRAW);
        const GLuint gridSSBO   = ComputeShader::setReadBuffer(&gridData[0], numThreads, GL_DYNAMIC_DRAW);
    	
        shader->bindBuffers(std::vector<GLuint>{ seedSSBO, gridSSBO });
        shader->use();
        shader->setUniform("gridDims", numDivs);
        shader->setUniform("numSeeds", GLuint(seeds.size()));
        shader->setSubroutineUniform(GL_COMPUTE_SHADER, "distanceUniform", distanceUniform[_dfunc]);
        shader->applyActiveSubroutines();
        shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

        if (/*fractParameters->_removeIsolatedRegions*/false)
        {
            this->removeIsolatedRegions(grid, seeds);
        }
        else
        {
            uint16_t* resultPointer = ComputeShader::readData(gridSSBO, uint16_t());
            resultBuffer = std::vector<uint16_t>(resultPointer, resultPointer + numThreads);
        }

        glDeleteBuffers(1, &seedSSBO);
        glDeleteBuffers(1, &gridSSBO);
    }

    void NaiveFracturer::setDistanceFunction(DistanceFunction dfunc)
	{
        _dfunc = dfunc;
    }
}
