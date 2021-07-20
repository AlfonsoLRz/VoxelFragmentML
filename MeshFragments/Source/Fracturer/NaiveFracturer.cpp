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
            //std::cout << seed.x << ", " << seed.y << ", " << seed.z << ", " << seed.w << std::endl;
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
    }

    void NaiveFracturer::init() {
        // Create and compile the buffers
        //_seedsBuffer.create();
        //_spaceBuffer.create();

        //// Generate space texture and link with space buffer
        //glGenTextures(1, &_spaceTexture);
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_BUFFER, _spaceTexture);

        //// Create voronoi compute shaders
        //opengl::ShaderObject comp(GL_COMPUTE_SHADER,
        //    #include "shaders/Naive.comp"
        //);

        //comp.create();
        //comp.compile();

        //std::cout << comp.getCompileLog() << std::endl;

        //// Create shaders program
        //_program.create();
        //_program.attach(comp);
        //_program.link();

        //// Destroy shaders object
        //comp.destroy();
    }

    void NaiveFracturer::destroy()
	{
        //_seedsBuffer.destroy();
        //_spaceBuffer.destroy();
        //_program.destroy();
    }

    void NaiveFracturer::build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, std::vector<uint16_t>& resultBuffer)
	{
        ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::NAIVE_FRACTURER);

        // Input data
        uvec3 numDivs       = grid.getNumSubdivisions();
        unsigned numThreads = numDivs.x * numDivs.y * numDivs.z;
        unsigned numGroups  = ComputeShader::getNumGroups(numThreads);
        uint16_t* gridData  = grid.data();

        const GLuint seedSSBO   = ComputeShader::setReadBuffer(seeds, GL_STATIC_DRAW);
        const GLuint gridSSBO   = ComputeShader::setReadBuffer(&gridData[0], numThreads, GL_STATIC_DRAW);
        const GLuint resSSBO    = ComputeShader::setWriteBuffer(uint16_t(), numThreads, GL_DYNAMIC_DRAW);
    	
        shader->bindBuffers(std::vector<GLuint>{ seedSSBO, gridSSBO, resSSBO });
        shader->use();
        shader->setUniform("gridDims", numDivs);
        shader->setUniform("numSeeds", GLuint(seeds.size()));
        shader->setSubroutineUniform(GL_COMPUTE_SHADER, "distanceUniform", "manhattanDistance");
        shader->applyActiveSubroutines();
        shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

        uint16_t* resultPointer = ComputeShader::readData(resSSBO, uint16_t());
        resultBuffer = std::vector<uint16_t>(resultPointer, resultPointer + numThreads);

        glDeleteBuffers(1, &seedSSBO);
        glDeleteBuffers(1, &gridSSBO);
        glDeleteBuffers(1, &resSSBO);
    }

    void NaiveFracturer::setDistanceFunction(DistanceFunction dfunc)
	{
        _dfunc = dfunc;
    }
}
