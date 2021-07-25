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
        typedef std::function<float(const ivec3& pos1, const ivec4& pos2)> DistFunction;
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
        void buildCPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters);

        /**
		*   Split up a volumentric object into fragments (CPU version).
		*   @param[in] grid Volumetric space we want to split into fragments
		*   @param[in] seed  Seeds used to generate fragments
		*/
        void buildGPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters);

    	/*
    	*  Removes fragments isolated with respect to the fragment where a seed was originally placed.
    	*/
        void removeIsolatedRegionsCPU(RegularGrid& grid, const std::vector<glm::uvec4>& seeds);

        /*
		*  Removes fragments isolated with respect to the fragment where a seed was originally placed (GPU version).
		*/
        void removeIsolatedRegionsGPU(const GLuint gridSSBO, RegularGrid& grid, const std::vector<glm::uvec4>& seeds);

    public:
    	/**
    	*   @brief Destructor. 
    	*/
        ~NaiveFracturer() {};

        /**
        *   Split up a volumentric object into fragments.
        *   @param[in] grid Volumetric space we want to split into fragments
        *   @param[in] seed  Seeds used to generate fragments
        */
        void build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters);

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
