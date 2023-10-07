#pragma once

#include "Geometry/3D/AABB.h"
#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Core/Model3D.h"

class WingedTriangleMesh
{
protected:
	struct WingedTriangle
	{
		Triangle3D						_triangle;
		unsigned						_cluster;
		unsigned						_face;
		std::unordered_set<unsigned>	_connectionVertex;
		std::unordered_set<unsigned>	_connectionEdge;
	};

protected:
	std::vector<std::vector<std::vector<std::vector<std::pair<unsigned, unsigned>>>>>	_pointGrid;
	std::vector<WingedTriangle>															_triangle;

	AABB					_aabb;									//!< Bounding box of the scene
	vec3					_cellSize;								//!< Size of each grid cell
	uvec3					_numDivs;								//!< Number of subdivisions of space between mininum and maximum point

protected:
	/**
	*	@brief Checks connectivity between faces.
	*/
	void checkVertexConnectivity();

	/**
	*	@return Index of grid cell to be filled.
	*/
	uvec3 getPositionIndex(const vec3& position);

	/**
	*	@return Index in grid array of a non-real position.
	*/
	unsigned getPositionIndex(int x, int y, int z) const;

public:
	/**
	*	@brief Constructor from a set of faces, given the vertices forming such faces.
	*/
	WingedTriangleMesh(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, const std::vector<unsigned>& isBoundary, const std::vector<float>& clusterIdx);

	/**
	*	@brief Destructor.
	*/
	virtual ~WingedTriangleMesh();
};

