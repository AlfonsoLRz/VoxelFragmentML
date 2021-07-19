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
	*	@return Index of grid cell to be filled.
	*/
	uvec3 getPositionIndex(const vec3& position);

public:
	/**
	*	@brief Constructor which specifies the area and the number of divisions of such area.
	*/
	RegularGrid(const AABB& aabb, uvec3 subdivisions);

	/**
	*	@brief Invalid copy constructor.
	*/
	RegularGrid(const RegularGrid& regulargrid) = delete;

	/**
	*	@brief  
	*/
	void fill(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, unsigned index, int numSamples);

	/**
	*	@brief Destructor.
	*/
	virtual ~RegularGrid();

	/**
	*	@brief Retrieves grid AABBs for rendering purposes. 
	*/
	void getAABBs(std::vector<AABB>& aabb);

	/**
	*	@return Number of subdivisions for every axis.
	*/
	uvec3 getNumSubdivisions() { return _numDivs; }

	/**
	*	@brief Inserts a new point in the grid.
	*/
	void insertPoint(const vec3& position, unsigned index);

	// ----------- External functions ----------

   /**
   * Get data pointer.
   * @return Internal data pointer
   */
    uint8_t* data();
    const uint8_t* data() const;
    ///@}

    /**
     * Read voxel.
     * @pre x in range [-1, size.x]
     * @pre y in range [-1, size.y]
     * @pre z in range [-1, size.z]
     * @param[in] x Voxel x coord
     * @param[in] y Voxel y coord
     * @param[in] z Voxel z coord
     * @return Read voxel value
     */
     ///@{
    const uint8_t& at(int x, int y, int z) const;
    uint8_t& at(int x, int y, int z);
    ///@}

    /**
     * Set voxel at position [x, y, z].
     * @pre x in range [-1, size.x]
     * @pre y in range [-1, size.y]
     * @pre z in range [-1, size.z]
     * @param[in] x Voxel x coord
     * @param[in] y Voxel y coord
     * @param[in] z Voxel z coord
     * @param[in] i Voxel new color index
     */
    void set(int x, int y, int z, uint8_t i);

    /**
     * Check if a voxel is occupied.
     * @pre x in range [-1, size.x]
     * @pre y in range [-1, size.y]
     * @pre z in range [-1, size.z]
     * @param[in] x Voxel x coord
     * @param[in] y Voxel y coord
     * @param[in] z Voxel z coord
     * @return True if the voxel is occupied, false if not
     */
    bool isOccupied(int x, int y, int z) const;

    /**
     * Check if a voxel is empty.
     * @pre x in range [-1, size.x]
     * @pre y in range [-1, size.y]
     * @pre z in range [-1, size.z]
     * @param[in] x Voxel x coord
     * @param[in] y Voxel y coord
     * @param[in] z Voxel z coord
     * @return True if the voxel is empty, false if not
     */
    bool isEmpty(int x, int y, int z) const;

    /**
     * Voxel space dimensions.
     * @return Space dimension
     */
    glm::uvec3 dims() const;

    /**
     * Voxel space dimensions including padding.
     * @return Space size including padding
     */
    glm::uvec3 size() const;

    /**
     * Space size in bytes.
     * @return size.x * size.y * size.z
     */
    size_t length() const;

    /**
     * Get color palette.
     * @return The space color palette
     */
    const palette_t& getPalette() const;

    /**
     * Set new color palette.
     * @param[in] palette The new color palette
     */
    void setPalette(const palette_t& palette);

    /**
     * Get subspace.
     * @param[in] aabb Subspace AABB
     */
    Space subspace(const AABB& aabb) const;

    /**
     * Reapint a subsapce.
     * @param[in] aabb      AABB of the subspace we want to repaint
     * @param[in] previous  Color we want to replace
     * @param[in] newcolor  color to replace with
     */
    void repaint(const AABB& aabb, uint8_t previous, uint8_t newcolor);

    /**
     * Computes AABB for every color index.
     * If there is no one voxel for a specific color index the AABB will be
     * ill formed.
     * @return a vector with 255 AABB, one per color index
     */
    std::vector<AABB> splitByColorIndex() const;

    /**
     * Ser all voxel as EMPTY excepts those with specified color index.
     * @param[in] color Color index filter
     */
    void filterByColorIndex(uint8_t color);

    /**
     * Set every voxel that is not EMPTY as FREE.
     */
    void homogenize();
};

