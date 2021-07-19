#pragma once

#include "Geometry/3D/AABB.h"
#include "Graphics/Core/Image.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/Texture.h"

/**
*	@file RegularGrid.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 09/02/2020
*/

#define VOXEL_EMPTY 0
#define VOXEL_FREE 1

/**
*	@brief Data structure which helps us to locate models on a terrain.
*/
class RegularGrid
{   
protected:
	uint16_t***		_grid;

	AABB			_aabb;									//!< Bounding box of the scene
	vec3			_cellSize;								//!< Size of each grid cell
	uvec3			_numDivs;								//!< Number of subdivisions of space between mininum and maximum point

protected:
	/**
	*	@brief Builds a 3D grid. 
	*/
	void buildGrid();

	/**
	*	@brief Destroys a 3D grid.
	*/
	void destroyGrid();
	
	/**
	*	@return Index of grid cell to be filled.
	*/
	uvec3 getPositionIndex(const vec3& position);

public:
	/**
	*	@brief Constructor which specifies the area and the number of divisions of such area.
	*/
	RegularGrid(const AABB& aabb, uvec3 subdivisions);

	/**
	*	@brief Constructor of an abstract regular grid with no notion of space size.
	*/
	RegularGrid(uvec3 subdivisions);

	/**
	*	@brief Invalid copy constructor.
	*/
	RegularGrid(const RegularGrid& regulargrid) = delete;

    /**
    *	@brief Destructor.
    */
    virtual ~RegularGrid();

	/**
	*	@brief  
	*/
	void fill(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, unsigned index, int numSamples);

	/**
	*	@brief Retrieves grid AABBs for rendering purposes. 
	*/
	void getAABBs(std::vector<AABB>& aabb);

	/**
	*	@brief Inserts a new point in the grid.
	*/
	void insertPoint(const vec3& position, unsigned index);

	// ----------- External functions ----------

    /**
    *   Get data pointer.
    *   @return Internal data pointer.
    */
    uint16_t*** data();

    /**
    *   Read voxel.
    *   @pre x in range [-1, size.x].
    *   @pre y in range [-1, size.y].
    *   @pre z in range [-1, size.z].
    *   @param[in] x Voxel x coord.
    *   @param[in] y Voxel y coord.
    *   @param[in] z Voxel z coord.
    *   @return Read voxel value.
    */
    uint16_t at(int x, int y, int z) const;

    /**
    *   Voxel space dimensions.
    *   @return Space dimension
    */
    glm::uvec3 getNumSubdivisions() const;

    /**
	*   Set every voxel that is not EMPTY as FREE.
	*/
    void homogenize();

    /**
	*   Check if a voxel is occupied.
	*   @pre x in range [-1, size.x].
	*   @pre y in range [-1, size.y].
	*   @pre z in range [-1, size.z].
	*   @param[in] x Voxel x coord.
	*   @param[in] y Voxel y coord.
	*   @param[in] z Voxel z coord.
	*   @return True if the voxel is occupied, false if not.
	*/
    bool isOccupied(int x, int y, int z) const;

    /**
    *   Check if a voxel is empty.
    *   @pre x in range [-1, size.x]
    *   @pre y in range [-1, size.y]
    *   @pre z in range [-1, size.z]
    *   @param[in] x Voxel x coord
    *   @param[in] y Voxel y coord
    *   @param[in] z Voxel z coord
    *   @return True if the voxel is empty, false if not
    */
    bool isEmpty(int x, int y, int z) const;

    /**
    *   Space size in bytes.
    *   @return size.x * size.y * size.z
    */
    size_t length() const;

    /**
	*   Set voxel at position [x, y, z].
	*   @pre x in range [-1, size.x].
	*   @pre y in range [-1, size.y].
	*    @pre z in range [-1, size.z].
	*   @param[in] x Voxel x coord.
	*   @param[in] y Voxel y coord.
	*   @param[in] z Voxel z coord.
	*   @param[in] i Voxel new color index.
	*/
    void set(int x, int y, int z, uint8_t i);
};

