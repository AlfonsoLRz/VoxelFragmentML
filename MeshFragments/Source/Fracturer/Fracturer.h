#pragma once

#include "DataStructures/RegularGrid.h"
#include "Graphics/Core/FractureParameters.h"
#include "Utilities/Singleton.h"

namespace fracturer {

    // Enumaration of diferente distance metrics
    enum DistanceFunction : GLuint {
        EUCLIDEAN_DISTANCE = 0,     // (layout = 0)
        MANHATTAN_DISTANCE = 1,     // (layout = 1)
        CHEBYSHEV_DISTANCE = 2      // (layout = 2)
    };

    /**
    *   Volumetric space fracturer.
    *   Interface that every fracturer must implement.
    *   @code Usage example
    *   ~~~~~~~~~~~~~~~{.cpp}
    *   my_fracturer.init();
    *   ...
    *   my_fracturer.build(my_voxspace, my_seeds);
    *  ...
    *   my_fracturer.destroy();
    *   ~~~~~~~~~~~~~~~
    */
    class Fracturer {
    public:
        /**
        *   Split up a volumentric object into fragments.
        *   @param[in] grid Volumetric space we want to split into fragments.
        *   @param[in] seed  Seeds used to generate fragments.
        */
        virtual void build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters) = 0;

        /**
        *   Set distance funcion.
        *   @param[in] dfunc Distance function.
        */
        virtual void setDistanceFunction(DistanceFunction dfunc) = 0;
    };

}
