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

#ifndef VOXFRACTURER_FRACTURER_SEEDER_H_
#define VOXFRACTURER_FRACTURER_SEEDER_H_

#include <limits>
#include <set>
#include <vector>
#include <random>
#include <exception>
#include <string>

#include <glm/glm.hpp>

#include "Fracturer.h"
#include "../vox/Space.h"

namespace fracturer {

    class Seeder {
    public:
        /**
         * Exception raised when the seeder cannot proceed
         * Reasons: MAX_TRIES have been exceed
         */
        class SeederSearchError : public std::runtime_error {
        public:
            explicit SeederSearchError(const std::string& msg) : std::runtime_error(msg) {  }
        };

        /** Exception raised when requested seeds surpasses MAX_SEEDS */
        class TooManySeedsError : public std::runtime_error {
        public:
            explicit TooManySeedsError(const std::string& msg) : std::runtime_error(msg) {  }
        };

        /** Maximun number of tries on seed search */
        static const int MAX_TRIES = 10000;

        /** Maximun number of seeds */
        static const int MAX_SEEDS = 254;

        /**
         * Merge seeds randomly until there are no extra seeds.
         * Merge criteria is minimun distance.
         * @param[in]    frags Fragments seeds
         * @param[inout] seeds Voronoi seeds
         */
        static void mergeSeeds(const std::vector<glm::uvec4>& frags,
            std::vector<glm::uvec4>& seeds, DistanceFunction dfunc);

        /**
         * Brute force generator of seeds using an uniform distribution
         * Just tries in the voxel space till it founds a "fill" voxel.
         * Why vec4 and not vec3? Because on GPU there is no vec3 memory aligment.
         * Warning! every seeds has: x, y, z, colorIndex. Min colorIndex is 2
         * becouse in Flood algorithm colorIndex 1 is reserved for 'free' voxel.
         */
        static std::vector<glm::uvec4> uniform(const vox::Space& space, unsigned int nseeds);
    };
}

#endif //VOXFRACTURER_FRACTURER_SEEDER_H_
