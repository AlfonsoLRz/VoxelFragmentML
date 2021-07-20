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

    void FloodFracturer::build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, std::vector<uint16_t>& resultBuffer) {
        grid.homogenize();

        // Set seeds
        for (auto& seed : seeds)
            grid.set(seed.x, seed.y, seed.z, seed.w);

        // Initialize first iteration frontier
        //_frontBuffer.bind();
        //_frontBuffer.setSubData(seeds.data(), seeds.size() * sizeof(glm::uvec4), 0);

        //// Number of frontier voxels
        //GLuint frontierSize = GLuint(seeds.size());

        //// Bind buffers
        //glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, _atomicBuffer.getHandler());

        //GLuint buffer1 = _frontBuffer.getHandler();
        //GLuint buffer2 = _newFrontBuffer.getHandler();

        //while (frontierSize > 0) {
        //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer1);
        //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffer2);

        //    // Set new frontier size
        //    _expandFront.uniform("readStackSize", frontierSize);

        //    // Dispach compute
        //    GLuint workGroups = GLuint(std::ceil(frontierSize / 64.0f));

        //    // Expand over every neigbour
        //    for (auto& expand : _dfunc == 1 ? VON_NEUMANN : MOORE) {
        //        _expandFront.uniform("expand", expand);
        //        glDispatchCompute(workGroups, 1, 1);
        //        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        //    }

        //    // Reset counter
        //    frontierSize = _resetAtomicCounter();

        //    // stack overflow?
        //    if (frontierSize > frontierBufferSize)
        //        throw StackOverflowError("Frontier has exceeded stack size");

        //    // Swap buffers
        //    std::swap(buffer1, buffer2);
        //}

        ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::FLOOD_FRACTURER);

        // Input data
        uvec3 numDivs       = grid.getNumSubdivisions();
        unsigned numCells   = numDivs.x * numDivs.y * numDivs.z;
        unsigned numGroups  = ComputeShader::getNumGroups(numCells);
        unsigned nullCount  = 0;
        unsigned stackSize  = seeds.size();
        uint16_t* gridData  = grid.data();

        const GLuint stack1SSBO     = ComputeShader::setWriteBuffer(uint16_t(), numCells, GL_DYNAMIC_DRAW);
        const GLuint stack2SSBO     = ComputeShader::setWriteBuffer(uint16_t(), numCells, GL_DYNAMIC_DRAW);
        const GLuint gridSSBO       = ComputeShader::setWriteBuffer(uint16_t(), numCells, GL_DYNAMIC_DRAW);
        const GLuint neighborSSBO   = ComputeShader::setReadBuffer(_dfunc == 1 ? VON_NEUMANN : MOORE, GL_STATIC_DRAW);
        const GLuint stackSizeSSBO  = ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);

    	// Load seeds as a subset
        ComputeShader::updateReadBufferSubset(stack1SSBO, seeds.data(), 0, seeds.size());

        shader->bindBuffers(std::vector<GLuint>{ gridSSBO, stack1SSBO, stack2SSBO, stackSizeSSBO, neighborSSBO });
        shader->use();
        shader->setUniform("numNeighbors", _dfunc == 1 ? GLuint(VON_NEUMANN.size()) : GLuint(MOORE.size()));

        while (stackSize > 0) 
        {
            ComputeShader::updateReadBuffer(stackSizeSSBO, &nullCount, 1, GL_DYNAMIC_DRAW);
        	
            shader->setUniform("gridDims", numDivs);
            shader->setUniform("stackSize", stackSize);
            shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
        }

        uint16_t* resultPointer = ComputeShader::readData(gridSSBO, uint16_t());
        resultBuffer = std::vector<uint16_t>(resultPointer, resultPointer + numCells);

        GLuint deleteBuffers[] = { stack1SSBO, stack2SSBO, gridSSBO, neighborSSBO, stackSize };
        glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);
    }

    void FloodFracturer::setDistanceFunction(DistanceFunction dfunc) {
        // EUCLIDEAN_DISTANCE is forbid
        if (dfunc == EUCLIDEAN_DISTANCE)
            throw std::invalid_argument("Flood method is not compatible with euclidean metric. Try with naive method.");

        _dfunc = dfunc;
    }
}