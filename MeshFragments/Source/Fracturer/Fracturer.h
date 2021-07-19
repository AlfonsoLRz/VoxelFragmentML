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

#ifndef VOXFRACTURER_FRACTURER_FRACTURER_H_
#define VOXFRACTURER_FRACTURER_FRACTURER_H_

#include <glad/glad.h>

#include "../util/Singleton.h"
#include "../vox/Space.h"

namespace fracturer {

    // Enumaration of diferente distance metrics
    enum DistanceFunction : GLuint {
        EUCLIDEAN_DISTANCE = 0, // (layout = 0)
        MANHATTAN_DISTANCE = 1, // (layout = 1)
        CHEBYSHEV_DISTANCE = 2  // (layout = 2)
    };

    /**
     * Volumetric space fracturer.
     * Interface that every fracturer must implement.
     * @code Usage example
     * ~~~~~~~~~~~~~~~{.cpp}
     * my_fracturer.init();
     *  ...
     *  my_fracturer.build(my_voxspace, my_seeds);
     *  ...
     * my_fracturer.destroy();
     * ~~~~~~~~~~~~~~~
     */
    class Fracturer {
    public:

        /** Initialize shaders, create objects and set OpenGL needed configuration. */
        virtual void init() = 0;

        /*** Free resources. */
        virtual void destroy() = 0;

        /**
         * Split up a volumentric object into fragments.
         * @param[in] space Volumetric space we want to split into fragments
         * @param[in] seed  Seeds used to generate fragments
         */
        virtual void build(vox::Space& space, const std::vector<glm::uvec4>& seeds) = 0;

        /**
         * Set distance funcion.
         * @param[in] dfunc Distance funcion
         */
        virtual void setDistanceFunction(DistanceFunction dfunc) = 0;
    };

}

#endif //VOXFRACTURER_FRACTURER_FRACTURER_H_
