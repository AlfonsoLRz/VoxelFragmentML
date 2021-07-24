#pragma once

#include "stdafx.h"

#include "DataStructures/RegularGrid.h"
#include "Fracturer.h"

namespace fracturer {

    class Seeder {
    public:
        /**
        *   Exception raised when the seeder cannot proceed.
        *   Reasons: MAX_TRIES have been exceed.
        */
        class SeederSearchError : public std::runtime_error {
        public:
            explicit SeederSearchError(const std::string& msg) : std::runtime_error(msg) {  }
        };

        static const int MAX_TRIES = 1000000;     //!< Maximun number of tries on seed search.

    	/**
    	*   @brief Creates seeds near the current ones. 
    	*/
        static std::vector<glm::uvec4> nearSeeds(const RegularGrid& grid, const std::vector<glm::uvec4>& frags, unsigned numSeeds, unsigned spreading);
    	
        /**
        *   Merge seeds randomly until there are no extra seeds.
        *   Merge criteria is minimun distance.
        *   @param[in]    frags Fragments seeds
        *   @param[inout] seeds Voronoi seeds
        */
        static void mergeSeeds(const std::vector<glm::uvec4>& frags, std::vector<glm::uvec4>& seeds, DistanceFunction dfunc);

        /**
        *   Brute force generator of seeds using an uniform distribution
        *   Just tries in the voxel space till it founds a "fill" voxel.
        *   Why vec4 and not vec3? Because on GPU there is no vec3 memory aligment.
        *   Warning! every seeds has: x, y, z, colorIndex. Min colorIndex is 2
        *   becouse in Flood algorithm colorIndex 1 is reserved for 'free' voxel.
        */
        static std::vector<glm::uvec4> uniform(const RegularGrid& grid, unsigned int nseeds);
    };
}
