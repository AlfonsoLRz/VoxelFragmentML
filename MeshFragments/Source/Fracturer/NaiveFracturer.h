#pragma once

#include "Geometry/3D/AABB.h"
#include "Graphics/Core/AABBSet.h"
#include "Fracturer.h"
#include "Seeder.h"
#include "Utilities/Singleton.h"

namespace fracturer {

    /**
     * Volumetric object fracturer using naive algorithm. NaiveFracturer
     * consumes less time and memory than FloodFracturer.
     */
    class NaiveFracturer : public Singleton<NaiveFracturer>, public Fracturer
	{
        friend class Singleton<NaiveFracturer>;
    	
        NaiveFracturer();
        ~NaiveFracturer() = default;

    public:

        /**
        * Remove isolated regions.
        * @param[inout] space Space where we want to remove isolated regions
        * @param[in]    seeds Voronoi seeds used to generate the model
        */
        static void removeIsolatedRegions(vox::Space& space, const std::vector<glm::uvec4>& seeds);

        /** You must invoke init() method before using NaiveFracturer. */
        void init();

        /** Release resources. You must invoke destroy() before OpenGL context is destroyed. */
        void destroy();

        /**
         * Split up a volumentric object into fragments.
         * @param[in] space Volumetric space we want to split into fragments
         * @param[in] seed  Seeds used to generate fragments
         */
        void build(vox::Space& space, const std::vector<glm::uvec4>& seeds);

        /**
         * Set distance funcion.
         * @param[in] dfunc Distance funcion
         */
        void setDistanceFunction(DistanceFunction dfunc);

    private:

        DistanceFunction      _dfunc;           //!< Inner distance metric
        GLuint                _spaceTexture;    //<! Texture used to sample space buffer in shader
        opengl::BufferObject  _spaceBuffer;     //<! Voxel space storage buffer object
        opengl::BufferObject  _seedsBuffer;     //<! Seeds storage buffer object
        opengl::ShaderProgram _program;         //<! Naive algorithm shaders program
    };

}

#endif //VOXFRACTURER_FRACTURER_NAIVE_FRACTURER_H_
