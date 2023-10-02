#pragma once

#include "DataStructures/RegularGrid.h"
#include "Fracturer.h"
#include "Utilities/HaltonEnum.h"
#include "Utilities/HaltonSampler.h"

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

    public:
        typedef std::function<void(int size)> RandomInitFunction;
        typedef std::unordered_map<uint16_t, RandomInitFunction> RandomInitUniformMap;

        typedef std::function<int(int min, int max, int index, int coord)> RandomFunction;
        typedef std::unordered_map<uint16_t, RandomFunction> RandomUniformMap;

        typedef std::function<float(float min, float max, int index, int coord)> RandomFunctionFloat;
        typedef std::unordered_map<uint16_t, RandomFunctionFloat> RandomUniformMapFloat;

        static Halton_sampler       _haltonSampler;
        static Halton_enum          _haltonEnum;

        static RandomInitUniformMap _randomInitFunction;
        static RandomUniformMap     _randomFunction;
        static RandomUniformMapFloat _randomFunctionFloat;

    protected:
        static const int            MAX_TRIES = 1000000;     //!< Maximun number of tries on seed search.

    public:
        /**
        *   @brief 
        */
        static void getFloatNoise(unsigned int maxBufferSize, unsigned int nseeds, int randomSeedFunction, std::vector<float>& noiseBuffer);

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
        static std::vector<glm::uvec4> uniform(const RegularGrid& grid, unsigned int nseeds, int randomSeedFunction);
    };
}
