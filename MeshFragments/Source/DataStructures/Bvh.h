#pragma once

#include "Graphics/Core/Model3D.h"

class Bvh
{
	struct StaticGPUData;
	struct VolatileGPUData;

protected:
	const static GLuint				BVH_BUILDING_RADIUS;			//!< Radius to search nearest neighbors in BVH building process

protected:
	AABB							_aabb;							//!< Axis aligned bounding box of the whole group		
	Model3D*						_model;
	std::vector<StaticGPUData>		_staticGPUData;	

protected:
	/**
	*	@brief Builds the buffers which are necessary to build the BVH.
	*/
	void aggregateSSBOData(VolatileGPUData*& volatileGPUData);

	/**
	*	@brief Builds the VAO which allows us to render the BVH levels.
	*/
	void buildBVHVAO(StaticGPUData* staticData, VolatileGPUData* gpuData);

	/**
	*	@brief Builds the input buffer which is neeeded as input array to generate BVH.
	*	@param sortedFaces GPU buffer where face indices are sortered according to their Morton codes.
	*/
	void buildClusterBuffer(VolatileGPUData* gpuData, StaticGPUData* staticData, const GLuint sortedFaces);

	/**
	*	@brief Computes the morton codes for each triangle boundind box.
	*/
	GLuint computeMortonCodes(StaticGPUData* staticData);

	/**
	*	@brief Rearranges the face array to sort it by morton codes.
	*	@param mortonCodes Computed morton codes from faces buffer.
	*/
	GLuint sortFacesByMortonCode(StaticGPUData* staticData, const GLuint mortonCodes);

public:
	/**
	*	@brief Constructor.
	*	@param modelMatrix Transformation to be applied for the whole group.
	*/
	Bvh(Model3D* model);

	/**
	*	@brief Destructor.
	*/
	virtual ~Bvh();

public:
	/**
	*	@brief
	*/
	struct VolatileGPUData
	{
		// [BVH]
		std::vector<Model3D::BVHCluster>	_cluster;						//!< BVH nodes

		// [GPU buffers]
		GLuint								_tempClusterSSBO;				//!< Temporary buffer for BVH construction
		GLuint								_mortonCodesSSBO;				//!< Morton codes for each triangle

		/**
		*	@brief Default constructor.
		*/
		VolatileGPUData();

		/**
		*	@brief Destructor.
		*/
		~VolatileGPUData();
	};

	/**
	*	@brief
	*/
	struct StaticGPUData
	{
		// [SSBO]
		GLuint		_groupGeometrySSBO;		//!< SSBO of group geometry
		GLuint		_groupTopologySSBO;		//!< SSBO of group faces
		GLuint		_groupMeshSSBO;			//!< SSBO of group meshes
		GLuint		_clusterSSBO;			//!< BVH nodes

		// [Metadata]
		GLuint		_numClusters;			//!< Number of clusters in BVH
		GLuint		_numTriangles;			//!< Replaces cluster array data to release memory

		/**
		*	@brief Default constructor.
		*/
		StaticGPUData();

		/**
		*	@brief Destructor.
		*/
		~StaticGPUData();
	};

};

