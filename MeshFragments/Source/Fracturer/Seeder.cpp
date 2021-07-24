#include "stdafx.h"
#include "Seeder.h"

namespace fracturer
{
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

    std::vector<glm::uvec4> Seeder::uniform(const RegularGrid& grid, unsigned int nseeds) {
        // Custom glm::uvec3 comparator
        auto comparator = [](const glm::uvec3& lhs, const glm::uvec3& rhs) {
            if      (lhs.x != rhs.x) return lhs.x < rhs.x;
            else if (lhs.y != rhs.y) return lhs.y < rhs.y;
            else                     return lhs.z < rhs.z;
        };

        // Set where to store seeds
        std::set<glm::uvec3, decltype(comparator)> seeds(comparator);
        // Current try number
        unsigned int attempt = 0;
        uvec3 numDivs = grid.getNumSubdivisions() - uvec3(2);

        // Bruteforce seed search
        while (seeds.size() != nseeds) {
            // Check attempt number
            if (attempt == MAX_TRIES)
                throw SeederSearchError("Max number of tries surpassed (" +
                    std::to_string(MAX_TRIES) + ")");

            // Generate random seed
            int x = rand() % numDivs.x + 1;
            int y = rand() % numDivs.y + 1;
            int z = rand() % numDivs.z + 1;
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
