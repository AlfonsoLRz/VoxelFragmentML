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

#ifndef VOXFRACTURER_FRACTURER_FLOOD_FRACTURER_H_
#define VOXFRACTURER_FRACTURER_FLOOD_FRACTURER_H_

#include <climits>

#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include "../opengl/BufferObject.h"
#include "../opengl/ShaderProgram.h"

#include "../util/Singleton.h"

#include "../vox/AABB.h"
#include "../vox/Space.h"

#include "Fracturer.h"
#include "Seeder.h"

namespace fracturer {

    /**
     * Volumetric object fracturer using flood algorithm. The main diference
     * between this and VoronoiFracturer is that FloodFracturer guarantees
     * every fragment is solid (VoronoiFracturer can generate `fragmented` fragments).
     */
    class FloodFracturer : public util::Singleton<FloodFracturer>, public Fracturer {

        // util::Singleton<FloodFracturer> needs access to the constructor and destructor
        friend class util::Singleton<FloodFracturer>;
        FloodFracturer();
        ~FloodFracturer() = default;

    public:

        /** Array with Von Neumann neighbourhood */
        static const std::vector<glm::ivec4> VON_NEUMANN;

        /** Array with MOORE neighbourhood */
        static const std::vector<glm::ivec4> MOORE;

        /** Exception thrown when voxel stack is overflow. */
        class StackOverflowError : public std::runtime_error {
        public:
            explicit StackOverflowError(const std::string& msg) : std::runtime_error(msg) {  }
        };

        /**
         * Initialize shaders, create objects and set OpenGL needed configuration.
         * You must invoke init() method before using FloodFracturer.
         */
        void init();

        /**
         * Free resources.
         * You must invoke init() method before using FloodFracturer again.
         */
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

        DistanceFunction _dfunc;    //!< Inner distance metric

        /** Reset atomic counter to zero and return previous value. */
        GLuint _resetAtomicCounter();

        GLuint                _spaceTexture;    //!< Texture used to sample space buffer in shader
        opengl::BufferObject  _spaceBuffer;     //!< Voxel space imageBuffer object

        opengl::BufferObject  _frontBuffer;     //!< First frontier buffer 
        opengl::BufferObject  _newFrontBuffer;  //!< Second frontier buffer 

        opengl::BufferObject  _atomicBuffer;    //!< Buffer for atomic variables 

        opengl::ShaderProgram _expandFront;     //!< Expand front shader program
    };

}

#endif //VOXFRACTURER_FRACTURER_FLOOD_FRACTURER_H_
