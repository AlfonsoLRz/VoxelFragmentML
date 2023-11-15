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
		GLuint  _numCells;
		GLuint  _neighborSSBO;
		GLuint  _stack1SSBO;
		GLuint  _stack2SSBO;
		GLuint  _stackSizeSSBO;

	protected:
		/**
		*   Constructor.
		*/
		FloodFracturer();

	public:
		/**
		*   Destructor.
		*/
		~FloodFracturer() { this->destroy(); };

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
		*   Split up a volumentric object into fragments.
		*   @param[in] grid Volumetric space we want to split into fragments
		*   @param[in] seed  Seeds used to generate fragments
		*/
		virtual void build(RegularGrid& grid, const std::vector<glm::uvec4>& seeds, FractureParameters* fractParameters);

		/**
		*   Free resources.
		*   You must invoke init() method before using FloodFracturer again.
		*/
		virtual void destroy();

		/**
		*   Initialize shaders, create objects and set OpenGL needed configuration.
		*   You must invoke init() method before using FloodFracturer.
		*/
		virtual void init(FractureParameters* fractParameters);

		/**
		*   @brief Allocates GPU memory.
		*/
		virtual void prepareSSBOs(FractureParameters* fractParameters);

		/**
		*   Set distance funcion.
		*   @param[in] dfunc Distance funcion
		*/
		virtual bool setDistanceFunction(DistanceFunction dfunc);

	private:

		DistanceFunction _dfunc;    //!< Inner distance metric
	};

}
