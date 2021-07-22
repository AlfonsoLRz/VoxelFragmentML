#pragma once

#include "DataStructures/RegularGrid.h"
#include "Geometry/3D/AABB.h"
#include "Fracturer.h"
#include "Seeder.h"

namespace fracturer {

    /**
    *   Volumetric object fracturer using naive algorithm. NaiveFracturer consumes less time and memory than FloodFracturer.
    */
    class NaiveFracturer : public Singleton<NaiveFracturer>, public Fracturer
	{
        friend class Singleton<NaiveFracturer>;
        typedef std::function<float(const vec3& pos1, const vec3& pos2)> DistFunction;
        typedef std::unordered_map<uint16_t, DistFunction> DistanceFunctionMap;

    protected:
        DistanceFunctionMap _distanceFunctionMap;
    
    protected:
        /**
        *   @brief Constructor.
        */
        NaiveFracturer();

        /**
		*   Split up a volumentric object into fragments (CPU version).
		*   @param[in] grid Volumetric space we want to split into fragments
		*   @param[in] seed  Seeds used to generate fragments
		*/
        void buildCPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, std::vector<uint16_t>& resultBuffer, FractureParameters* fractParameters);

        /**
		*   Split up a volumentric object into fragments (CPU version).
		*   @param[in] grid Volumetric space we want to split into fragments
		*   @param[in] seed  Seeds used to generate fragments
		*/
        void buildGPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, std::vector<uint16_t>& resultBuffer, FractureParameters* fractParameters);

    public:
    	/**
    	*   @brief Destructor. 
    	*/
        ~NaiveFracturer() {};
    	
        /**
        *   Remove isolated regions.
        *   @param[inout] grid Space where we want to remove isolated regions
        *   @param[in]    seeds Voronoi seeds used to generate the model
        */
        static void removeIsolatedRegions(RegularGrid& grid, const std::vector<glm::uvec4>& seeds);

        /**
        *   Split up a volumentric object into fragments.
        *   @param[in] grid Volumetric space we want to split into fragments
        *   @param[in] seed  Seeds used to generate fragments
        */
        void build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, std::vector<uint16_t>& resultBuffer, FractureParameters* fractParameters);

        /**
        *   Set distance funcion.
        *   @param[in] dfunc Distance function.
        */
        void setDistanceFunction(DistanceFunction dfunc);

    private:

        DistanceFunction      _dfunc;           //!< Inner distance metric
        GLuint                _spaceTexture;    //<! Texture used to sample space buffer in shader
    };
}
