#include "stdafx.h"
#include "NaiveFracturer.h"

namespace fracturer {
    NaiveFracturer::NaiveFracturer() : _dfunc(EUCLIDEAN_DISTANCE), _spaceTexture(0)
    {
	    
    }

    void NaiveFracturer::removeIsolatedRegions(vox::Space& space, const std::vector<glm::uvec4>& seeds)
	{
        // Final 3D image
        vox::Space dest(space.dims().x, space.dims().y, space.dims().z);

        // Linked list where to push/pop voxels
        std::list<glm::uvec4> front(seeds.begin(), seeds.end());

        for (glm::uvec4 seed : seeds) 
        {
            dest.set(seed.x, seed.y, seed.z, seed.w);
            //std::cout << seed.x << ", " << seed.y << ", " << seed.z << ", " << seed.w << std::endl;
        }

        // Iterate while voxel list is not empty
        while (!front.empty()) {
            // Get voxel
            glm::uvec4 v = front.front();

            // Expand front
            #define expand(dx, dy, dz)\
                if (space.at(v.x + (dx), v.y + (dy), v.z + (dz)) ==\
                    v.w && dest.at(v.x + (dx), v.y +(dy), v.z + (dz)) == vox::Space::VOXEL_EMPTY) {\
                    front.push_back(glm::uvec4(v.x + (dx), v.y + (dy), v.z + (dz), v.w));\
                    dest.set(v.x + (dx), v.y + (dy), v.z + (dz), v.w);\
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
        space = std::move(dest);
    }

    void NaiveFracturer::init() {
        // Create and compile the buffers
        _seedsBuffer.create();
        _spaceBuffer.create();

        // Generate space texture and link with space buffer
        glGenTextures(1, &_spaceTexture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, _spaceTexture);

        // Create voronoi compute shaders
        opengl::ShaderObject comp(GL_COMPUTE_SHADER,
            #include "shaders/Naive.comp"
        );

        comp.create();
        comp.compile();

        std::cout << comp.getCompileLog() << std::endl;

        // Create shaders program
        _program.create();
        _program.attach(comp);
        _program.link();

        // Destroy shaders object
        comp.destroy();
    }

    void NaiveFracturer::destroy() {
        _seedsBuffer.destroy();
        _spaceBuffer.destroy();
        _program.destroy();
    }

    void NaiveFracturer::build(vox::Space &space, const std::vector<glm::uvec4>& seeds) {
        // Prepare uniforms
        const glm::uvec3 size(space.size().y * space.size().z, space.size().z, 1);
        const glm::uvec3 dim = space.dims();

        // Use program and set uniforms
        _program.use();
        _program.uniform("nseeds",          GLuint(seeds.size()));
        _program.uniform("size",            size);
        _program.uniform("dim",             dim);
        _program.uniform("voxspaceSampler", GLint(0)); // Texture unit zero
        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, (GLuint*)&_dfunc);

        // Set Seeds data
        _seedsBuffer.bind();
        _seedsBuffer.setData(seeds.data(), seeds.size() * sizeof(glm::uvec4), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _seedsBuffer.getHandler());

        // Set space data
        _spaceBuffer.bind();
        _spaceBuffer.setData(space.data(), space.length(), GL_DYNAMIC_DRAW);

        // Bind texture
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, _spaceBuffer.getHandler());
        glBindImageTexture(3, _spaceTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

        // Compute number of work groups
        GLuint workGroupsX = GLuint(std::ceil(space.dims().x / 8.0f));
        GLuint workGroupsY = GLuint(std::ceil(space.dims().y / 8.0f));
        GLuint workGroupsZ = GLuint(std::ceil(space.dims().z / 8.0f));

        // Dispatch compute shader and wait until finalization
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // Unbind buffers
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);

        // Read back result
        glBindBuffer(GL_TEXTURE_BUFFER, _spaceBuffer.getHandler());
        uint8_t* ptr = (uint8_t*) glMapBuffer(GL_TEXTURE_BUFFER, GL_READ_ONLY);
        memcpy(space.data(), ptr, space.length());
        glUnmapBuffer(GL_TEXTURE_BUFFER);
    }

    void NaiveFracturer::setDistanceFunction(DistanceFunction dfunc) {
        _dfunc = dfunc;
    }
}
