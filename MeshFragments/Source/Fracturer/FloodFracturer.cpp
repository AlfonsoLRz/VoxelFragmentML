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

    }

    void FloodFracturer::init()
	{

    }

    void FloodFracturer::destroy()
	{
    }

    void FloodFracturer::build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters) {
        grid.homogenize();

        // Set seeds
        for (auto& seed : seeds)
            grid.set(seed.x, seed.y, seed.z, seed.w);
    	
        ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::FLOOD_FRACTURER);

        // Input data
        uvec3 numDivs       = grid.getNumSubdivisions();
        unsigned numCells   = numDivs.x * numDivs.y * numDivs.z;
        unsigned nullCount  = 0;
        unsigned stackSize  = seeds.size();
        uint16_t* gridData  = grid.data();
        unsigned numNeigh   = _dfunc == 1 ? GLuint(VON_NEUMANN.size()) : GLuint(MOORE.size());

        GLuint stack1SSBO           = ComputeShader::setWriteBuffer(GLuint(), numCells, GL_DYNAMIC_DRAW);
        GLuint stack2SSBO           = ComputeShader::setWriteBuffer(GLuint(), numCells, GL_DYNAMIC_DRAW);
        const GLuint gridSSBO       = ComputeShader::setReadBuffer(&gridData[0], numCells, GL_DYNAMIC_DRAW);
        const GLuint neighborSSBO   = ComputeShader::setReadBuffer(_dfunc == 1 ? VON_NEUMANN : MOORE, GL_STATIC_DRAW);
        const GLuint stackSizeSSBO  = ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);

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

            shader->bindBuffers(std::vector<GLuint>{ gridSSBO, stack1SSBO, stack2SSBO, stackSizeSSBO, neighborSSBO });
            shader->setUniform("stackSize", stackSize);
            shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

            stackSize = *ComputeShader::readData(stackSizeSSBO, GLuint());

            //  Swap buffers
		    std::swap(stack1SSBO, stack2SSBO);
        }

        uint16_t* resultPointer = ComputeShader::readData(gridSSBO, uint16_t());
        std::vector<uint16_t> resultBuffer = std::vector<uint16_t>(resultPointer, resultPointer + numCells);
        grid.swap(resultBuffer);

        GLuint deleteBuffers[] = { stack1SSBO, stack2SSBO, gridSSBO, neighborSSBO, stackSize };
        glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);
    }

    void FloodFracturer::setDistanceFunction(DistanceFunction dfunc)
	{
        // EUCLIDEAN_DISTANCE is forbid
        if (dfunc == EUCLIDEAN_DISTANCE)
            throw std::invalid_argument("Flood method is not compatible with euclidean metric. Try with naive method.");

        _dfunc = dfunc;
    }
}