//    Copyright(C) 2019, 2020 José María Cruz Lorite
//
//    This file is part of voxfracturer.
//
//    voxfracturer is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    voxfracturer is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with voxfracturer.  If not, see <https://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "FloodFracturer.h"

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

    FloodFracturer::FloodFracturer()
        : _dfunc(MANHATTAN_DISTANCE)
        , _spaceTexture(0)
/*        , _spaceBuffer(GL_TEXTURE_BUFFER)
        , _frontBuffer(GL_SHADER_STORAGE_BUFFER)
        , _newFrontBuffer(GL_SHADER_STORAGE_BUFFER)
        , _atomicBuffer(GL_ATOMIC_COUNTER_BUFFER)
        , _expandFront()*/ {       

    }

    void FloodFracturer::init() {
        //// Create buffers
        //_frontBuffer.create();
        //_newFrontBuffer.create();
        //_spaceBuffer.create();
        //_atomicBuffer.create();

        //// Initialize atomic buffer
        //_atomicBuffer.bind();
        //_atomicBuffer.setData(nullptr, sizeof(GLuint), GL_DYNAMIC_COPY);
        //_atomicBuffer.unbind();

        //// Generate space texture and link with space buffer
        //glGenTextures(1, &_spaceTexture);

        //// Initialize shader programs
        //_initShaderProgram(_expandFront,
        //    #include "shaders/ExpandFront.comp"
        //);
    }

    void FloodFracturer::destroy() {
        //_expandFront.destroy();
        //_frontBuffer.destroy();
        //_newFrontBuffer.destroy();
        //_atomicBuffer.destroy();
        //_spaceBuffer.destroy();
        glDeleteTextures(1, &_spaceTexture);
    }

    GLuint FloodFracturer::_resetAtomicCounter() {
        //_atomicBuffer.bind();

        // Map buffer memory
        GLuint *ac = (GLuint*) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
        GLuint value = *ac;
        *ac = 0;

        // Unmap the buffer
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        //_atomicBuffer.unbind();

        return value;
    }

    void FloodFracturer::build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds) {
        //space.homogenize();

        //// Set seeds
        //for (auto& seed : seeds)
        //    space.set(seed.x, seed.y, seed.z, seed.w);

        //// Prepare uniforms
        //const glm::uvec3 size(space.size().y * space.size().z, space.size().z, 1);

        //// Set program uniforms
        //_expandFront.use();
        //_expandFront.uniform("size",            size);
        //_expandFront.uniform("voxspaceSampler", GLint(0)); // Texture unit zero

        //// Set space buffer data
        //_spaceBuffer.bind();
        //_spaceBuffer.setData(space.data(), space.length(), GL_DYNAMIC_DRAW);

        //// Bind texture
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_BUFFER, _spaceTexture);
        //glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, _spaceBuffer.getHandler());
        //glBindImageTexture(3, _spaceTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

        //// Set frontier size
        //GLuint frontierBufferSize = GLuint(space.size().x * space.size().y * space.size().z / 8.0f);

        //_frontBuffer.bind();
        //_frontBuffer.setData(nullptr, frontierBufferSize * sizeof(glm::uvec4), GL_STREAM_COPY);
        //_newFrontBuffer.bind();
        //_newFrontBuffer.setData(nullptr, frontierBufferSize * sizeof(glm::uvec4), GL_STREAM_COPY);

        //// Initialize first iteration frontier
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

        //// Read back result
        //glBindBuffer(GL_TEXTURE_BUFFER, _spaceBuffer.getHandler());
        //uint8_t* ptr = (uint8_t*) glMapBuffer(GL_TEXTURE_BUFFER, GL_READ_ONLY);
        //memcpy(space.data(), ptr, space.length());
        //glUnmapBuffer(GL_TEXTURE_BUFFER);

        //// Unbind buffers
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
        //glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, 0);
        //glBindImageTexture(3, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);
        //glBindImageTexture(4, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);
    }

    void FloodFracturer::setDistanceFunction(DistanceFunction dfunc) {
        // EUCLIDEAN_DISTANCE is forbid
        if (dfunc == EUCLIDEAN_DISTANCE)
            throw std::invalid_argument("Flood fraturing is not compatible with euclidean metric. Try with naive method.");

        _dfunc = dfunc;
    }
}