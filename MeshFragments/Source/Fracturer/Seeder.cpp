#include "stdafx.h"
#include "Seeder.h"

namespace fracturer {
    
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

        // Bruteforce seed searcg
        while (seeds.size() != nseeds) {
            // Check attempt number
            if (attempt == MAX_TRIES)
                throw SeederSearchError("Max number of tries surpassed (" +
                    std::to_string(MAX_TRIES) + ")");

            // Generate random seed
            int x = rand() % grid.getNumSubdivisions().x;
            int y = rand() % grid.getNumSubdivisions().y;
            int z = rand() % grid.getNumSubdivisions().z;
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
        std::vector<glm::uvec4> ret;

        // Generate array of seed
        unsigned int nseed = VOXEL_FREE + 1;         // 2 because first seed id must be greater than 1
    	
        for (glm::uvec3 seed : seeds)
            ret.push_back(glm::uvec4(seed, nseed++));

        return ret;
    }
}
