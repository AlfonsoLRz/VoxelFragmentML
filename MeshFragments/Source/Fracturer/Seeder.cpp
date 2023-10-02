#include "stdafx.h"
#include "Seeder.h"

namespace fracturer
{
    Halton_sampler  Seeder::_haltonSampler;
    Halton_enum     Seeder::_haltonEnum (1, 1);

    fracturer::Seeder::RandomInitUniformMap Seeder::_randomInitFunction = {
        { FractureParameters::STD_UNIFORM, [](int size) -> void {}},
        { FractureParameters::HALTON, [](int size) -> void { Seeder::_haltonSampler.init_faure(); Seeder::_haltonEnum = Halton_enum(size, 1); }}				// Enumerate samples per pixel for the given resolution,
    };

    fracturer::Seeder::RandomUniformMap Seeder::_randomFunction = {
        { FractureParameters::STD_UNIFORM, [](int min, int max, int index, int coord) -> int { return RandomUtilities::getUniformRandomInt(min, max); }},
        { FractureParameters::HALTON, [](int min, int max, int index, int coord) -> int { return int(Seeder::_haltonSampler.sample(coord, index) * (max - min) + min); }},
    };

    fracturer::Seeder::RandomUniformMapFloat Seeder::_randomFunctionFloat = {
        { FractureParameters::STD_UNIFORM, [](float min, float max, int index, int coord) -> float { return RandomUtilities::getUniformRandom(min, max); }},
        { FractureParameters::HALTON, [](float min, float max, int index, int coord) -> float { return Seeder::_haltonSampler.sample(coord, index) * (max - min) + min; }},
    };

    void Seeder::getFloatNoise(unsigned int maxBufferSize, unsigned int nseeds, int randomSeedFunction, std::vector<float>& noiseBuffer)
    {
        _randomInitFunction[randomSeedFunction](maxBufferSize);

        for (unsigned int seed = 0; seed < nseeds; seed += 2)
        {
            noiseBuffer.push_back(_randomFunctionFloat[randomSeedFunction](.0f, 1.0f, seed / 2, 0));
            noiseBuffer.push_back(_randomFunctionFloat[randomSeedFunction](.0f, 1.0f, seed / 2, 1));
        }
    }

    std::vector<glm::uvec4> Seeder::nearSeeds(const RegularGrid& grid, const std::vector<glm::uvec4>& frags, unsigned numSeeds, unsigned spreading)
	{
        // Custom glm::uvec3 comparator
        auto comparator = [](const glm::uvec3& lhs, const glm::uvec3& rhs) {
            if (lhs.x != rhs.x) return lhs.x < rhs.x;
            else if (lhs.y != rhs.y) return lhs.y < rhs.y;
            else                     return lhs.z < rhs.z;
        };

        // Set where to store seeds
        std::set<glm::uvec3, decltype(comparator)> seeds(comparator);
        unsigned maxFragmentsSeed = (numSeeds / frags.size()) * 2, currentSeeds, nseeds;
        uvec3 numDivs = grid.getNumSubdivisions(), numDivs2 = numDivs / uvec3(2);

		for (const uvec4& frag: frags)
		{
            nseeds = frags.size() == 1 ? numSeeds : glm::clamp(RandomUtilities::getUniformRandomInt(1, maxFragmentsSeed), 1, int(numSeeds));
            currentSeeds = 0;
			
            // Bruteforce seed search
            while (currentSeeds != nseeds) {
                // Generate random seed
                int x = numDivs2.x - RandomUtilities::getBiasedRandomInt(0, numDivs.x, spreading);
                int y = numDivs2.y - RandomUtilities::getBiasedRandomInt(0, numDivs.y, spreading);
                int z = numDivs2.z - RandomUtilities::getBiasedRandomInt(0, numDivs.z, spreading);

                x = (frag.x + x + numDivs.x) % numDivs.x;
                y = (frag.y + y + numDivs.y) % numDivs.y;
                z = (frag.z + z + numDivs.z) % numDivs.z;
            	
                glm::uvec3 voxel(x, y, z);

                // Is occupied the voxel?
                bool occupied = grid.isOccupied(x, y, z);

                // Is repeated?
                bool isFree = seeds.find(voxel) == seeds.end();

                if (occupied && isFree)
                {
                    seeds.insert(voxel);
                    ++currentSeeds;
                }
            }

            numSeeds -= nseeds;
		}

        // Array of generated seeds
        std::vector<glm::uvec4> result;

        // Generate array of seed
        unsigned int nseed = VOXEL_FREE + 1;         // 2 because first seed id must be greater than 1

        for (glm::uvec3 seed : seeds)
            result.push_back(glm::uvec4(seed, nseed++));

        return result;
	}
	
    void Seeder::mergeSeeds(const std::vector<glm::uvec4>& frags, std::vector<glm::uvec4>& seeds, DistanceFunction dfunc) {
        for (auto& seed : seeds) 
        {
            float min   = std::numeric_limits<float>::max();
            int nearest = -1;
        	
            for (unsigned int i = 0; i < frags.size(); ++i) 
            {
                glm::vec3 f = frags[i];
                glm::vec3 s = glm::vec3(seed);
                float distance = .0f;

                switch (dfunc) {
                case EUCLIDEAN_DISTANCE:
                    distance = glm::distance(s, f);
                    break;
                case MANHATTAN_DISTANCE:
                    distance = abs(s.x - f.x) + abs(s.y - f.y) + abs(s.z - f.z);
                    break;
                case CHEBYSHEV_DISTANCE:
                    distance = glm::max(abs(s.x - f.x), glm::max(abs(s.y - f.y), abs(s.z - f.z)));
                    break;
                }

                if (distance < min) 
                {
                    min = distance;
                    nearest = i;
                }
            }

            // Nearest found?
            seed.w = frags[nearest].w;
        }
    }

    std::vector<glm::uvec4> Seeder::uniform(const RegularGrid& grid, unsigned int nseeds, int randomSeedFunction) {
        // Custom glm::uvec3 comparator
        auto comparator = [](const glm::uvec3& lhs, const glm::uvec3& rhs) {
            if      (lhs.x != rhs.x) return lhs.x < rhs.x;
            else if (lhs.y != rhs.y) return lhs.y < rhs.y;
            else                     return lhs.z < rhs.z;
        };

        // Set where to store seeds
        std::set<glm::uvec3, decltype(comparator)> seeds(comparator);
        // Current try number
        uvec3 numDivs = grid.getNumSubdivisions() - uvec3(2);
        unsigned int attempt = 0, maxDim = glm::max(numDivs.x, glm::max(numDivs.y, numDivs.z));

        _randomInitFunction[randomSeedFunction](maxDim);

        // Bruteforce seed search
        while (seeds.size() != nseeds) {
            // Check attempt number
            if (attempt == MAX_TRIES)
                throw SeederSearchError("Max number of tries surpassed (" + std::to_string(MAX_TRIES) + ")");

            // Generate random seed
            int x = _randomFunction[randomSeedFunction](0, numDivs.x + 1, attempt, 0);
            int y = _randomFunction[randomSeedFunction](0, numDivs.y + 1, attempt, 1);
            int z = _randomFunction[randomSeedFunction](0, numDivs.z + 1, attempt, 2);
            glm::uvec3 voxel(x, y, z);

            // Is occupied the voxel?
            bool occupied = grid.isOccupied(x, y, z);

            // Is repeated?
            bool isFree = seeds.find(voxel) == seeds.end();

            if (occupied && isFree)
                seeds.insert(voxel);

            attempt++;
        }

        // Array of generated seeds
        std::vector<glm::uvec4> result;

        // Generate array of seed
        unsigned int nseed = VOXEL_FREE + 1;         // 2 because first seed id must be greater than 1
    	
        for (glm::uvec3 seed : seeds)
            result.push_back(glm::uvec4(seed, nseed++));

        return result;
    }
}
