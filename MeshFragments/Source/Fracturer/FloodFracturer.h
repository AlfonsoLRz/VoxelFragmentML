#pragma once

#include "Fracturer.h"
#include "Seeder.h"

namespace fracturer {

    /**
    *   Volumetric object fracturer using flood algorithm. The main diference
    *   between this and VoronoiFracturer is that FloodFracturer guarantees
    *   every fragment is solid (VoronoiFracturer can generate `fragmented` fragments).
    */
    class FloodFracturer : public Singleton<FloodFracturer>, public Fracturer {

        // Singleton<FloodFracturer> needs access to the constructor and destructor
        friend class Singleton<FloodFracturer>;

    protected:
    	/**
    	*   Constructor. 
    	*/
        FloodFracturer();

    public:
        /**
		*   Destructor.
		*/
        ~FloodFracturer() = default;
    	
        /**
        *   Array with Von Neumann neighbourhood.
        */
        static const std::vector<glm::ivec4> VON_NEUMANN;

        /**
        *   Array with MOORE neighbourhood.
        */
        static const std::vector<glm::ivec4> MOORE;

        /**
        *   Exception thrown when voxel stack is overflow.
        */
        class StackOverflowError : public std::runtime_error
    	{
        public:
            explicit StackOverflowError(const std::string& msg) : std::runtime_error(msg) {  }
        };

        /**
        *   Initialize shaders, create objects and set OpenGL needed configuration.
        *   You must invoke init() method before using FloodFracturer.
        */
        void init();

        /**
        *   Free resources.
        *   You must invoke init() method before using FloodFracturer again.
        */
        void destroy();

        /**
        *   Split up a volumentric object into fragments.
        *   @param[in] grid Volumetric space we want to split into fragments
        *   @param[in] seed  Seeds used to generate fragments
        */
        void build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters);

        /**
        *   Set distance funcion.
        *   @param[in] dfunc Distance funcion
        */
        void setDistanceFunction(DistanceFunction dfunc);

    private:

        DistanceFunction _dfunc;    //!< Inner distance metric
    };

}
