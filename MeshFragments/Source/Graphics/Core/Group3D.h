#pragma once

#include "Geometry/3D/AABB.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/TriangleSet.h"

/**
*	@file Group3D.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/09/2019
*/

#define BVH_NODE_INDICES 36					// 12 LINES x 2 VERTICES + 12 (RESTART_PRIMITIVE_INDEX)

/**
*	@brief Wrapper for several 3d models which inherites from Model3D.
*/
class Group3D: public Model3D
{
public:
	struct GroupData;
	struct StaticGPUData;
	struct VolatileGPUData;

protected:
	const static GLuint					BVH_BUILDING_RADIUS;			//!< Radius to search nearest neighbors in BVH building process
		
protected:
	AABB								_aabb;							//!< Boundaries of all those objects defined behind this group
	std::vector<ModelComponent*>		_globalModelComp;				//!< Wraps every model component from the group
	std::vector<Model3D*>				_objects;						//!< Elements which take part of the group
	TriangleSet*						_triangleSet;					//!< Set of faces from collision tests

	// [Scene data]
	std::vector<StaticGPUData*>			_staticGPUData;					//!<

	// [Rendering status]
	std::vector<VAO*>					_bvhVAO;						//!< VAO which allows us to render current tree level

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
	*	@brief Computes the AABB which wraps all the triangles contained in this group.
	*	@param triangleBufferID To compute the AABB we need to define the triangle buffer in GPU, so that is saved to be reused.
	*/
	virtual AABB computeAABB(GroupData* groupData, StaticGPUData* staticData);

	/**
	*	@brief Computes the morton codes for each triangle boundind box.
	*/
	GLuint computeMortonCodes(GroupData* groupData, StaticGPUData* staticData);

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
	Group3D(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Destructor.
	*/
	virtual ~Group3D();

	/**
	*	@brief Adds a new component in the group array.
	*/
	void addComponent(Model3D* object);

	/**
	*	@brief Builds the BVH from scratch once the group is loaded.
	*/
	void generateBVH(std::vector<StaticGPUData*>& sceneData, bool buildVisualization = false);

	/**
	*	@brief Loads all those components who belong to this group, applying the model matrix linked to such group.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Assigns an id for a model component and registers it into an array.
	*/
	void registerModelComponent(ModelComponent* modelComp);

	/**
	*	@brief Extension from registerScene for inner nodes which are also groups.
	*/
	virtual void registerModelComponentGroup(Group3D* group);

	/**
	*	@brief Iterates through the group content to assign an id for each model component.
	*/
	void registerScene();

	// ------------------------- Getters -------------------------------

	/**
	*	@return Bounding volume of complete scene.
	*/
	AABB getAABB() { return _aabb; }

	/**
	*	@return Model component which is identifier by id param.
	*/
	ModelComponent* getModelComponent(unsigned id);

	/**
	*	@return Pointer of vector with all registered model components.
	*/
	std::vector<ModelComponent*>* getRegisteredModelComponents() { return &_globalModelComp; }

	// ---------------------------- Rendering ---------------------------------
	
	/**
	*	@brief Renders the model as a set of lines.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of points.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles with no texture as we only want to retrieve the depth.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

public:
	/**
	*	@brief
	*/
	struct VolatileGPUData
	{
		// [BVH]
		std::vector<BVHCluster>			_cluster;						//!< BVH nodes

		// [GPU buffers]
		GLuint							_tempClusterSSBO;				//!< Temporary buffer for BVH construction
		GLuint							_mortonCodesSSBO;				//!< Morton codes for each triangle

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
	struct GroupData
	{
		// [CPU generated data]
		AABB							_aabb;				//!< Wrapper box
		std::vector<VertexGPUData>		_geometry;			//!< Global geometry
		std::vector<FaceGPUData>		_triangleMesh;		//!< Global face data
		std::vector<MeshGPUData>		_meshData;			//!< Global meshes
	};

	/**
	*	@brief 
	*/
	struct StaticGPUData
	{
		// [SSBO]
		GLuint		_groupGeometrySSBO;						//!< SSBO of group geometry
		GLuint		_groupTopologySSBO;						//!< SSBO of group faces
		GLuint		_groupMeshSSBO;							//!< SSBO of group meshes
		GLuint		_clusterSSBO;							//!< BVH nodes

		// [Metadata]
		GLuint		_numClusters;							//!<
		GLuint		_numTriangles;							//!< Replaces cluster array data to release memory

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

