#pragma once

#include "Geometry/3D/AABB.h"
#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Core/Model3D.h"

class WingedTriangleMesh
{
protected:
	struct WingedTriangle
	{
		Triangle3D							_triangle;
		std::unordered_map<unsigned, float>	_cluster;
		unsigned							_face, _finalCluster;
		std::unordered_set<WingedTriangle*>	_connectionVertex;
	};

protected:
	std::vector<std::vector<std::vector<std::vector<std::pair<WingedTriangle*, unsigned>>>>>	_pointGrid;
	std::vector<WingedTriangle*>																_triangle;

	AABB					_aabb;									//!< Bounding box of the scene
	vec3					_cellSize;								//!< Size of each grid cell
	uvec3					_numDivs;								//!< Number of subdivisions of space between mininum and maximum point

protected:
	/**
	*	@brief Checks connectivity between faces.
	*/
	void checkVertexConnectivity();

	/**
	*	@brief Erases triangles whose neighbors and themselves belong to the same cluster.
	*/
	void eraseFlatTriangles();

	/**
	*	@brief Retrieves clusters in a face and its neighbours.	
	*/
	void getClusters(unsigned idx, std::unordered_set<unsigned>& clusterIdx);

	/**
	*	@return Index of grid cell to be filled.
	*/
	uvec3 getPositionIndex(const vec3& position);

	/**
	*	@return Index in grid array of a non-real position.
	*/
	unsigned getPositionIndex(int x, int y, int z) const;

	/**
	*	@return True if triangle is surrounded by other clusters.
	*/
	bool processTriangle(unsigned idx);

public:
	/**
	*	@brief Constructor from a set of faces, given the vertices forming such faces.
	*/
	WingedTriangleMesh(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, std::vector<std::unordered_map<unsigned, float>> faceClusterOccupancy);

	/**
	*	@brief Destructor.
	*/
	virtual ~WingedTriangleMesh();

	/**
	*	@brief 
	*/
	void computeHardCluster(float smoothness = 1.0f);

	/**
	*	@brief
	*/
	void computeSoftCluster(float threshold = 1.0f);

	/**
	*	@brief 
	*/
	void getFaceCluster(unsigned numFaces, std::vector<float>& clusterIdx);
};

