#pragma once

#include "Fracturer/Seeder.h"
#include "Geometry/3D/PointCloud3D.h"
#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Core/Model3D.h"
#include "objloader/OBJ_Loader.h"

/**
*	@file CADModel.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 11/28/2020
*/

#define COMMENT_CHAR "#"
#define CGAL_SIMPLIFICATION false

/**
*	@brief Model loaded from an OBJ file.
*/
class CADModel: public Model3D
{
protected:
	static std::unordered_map<std::string, std::unique_ptr<Material>> _cadMaterials;
	static std::unordered_map<std::string, std::unique_ptr<Texture>> _cadTextures;

public:
	const static std::string BINARY_EXTENSION;				//!< File extension for binary models
	const static std::string OBJ_EXTENSION;					//!< File extension for original OBJ models

protected:
	AABB			_aabb;									//!< Boundaries 
	std::string		_filename;								//!< File path (without extension)
	bool			_fuseComponents;						//!< Fuse all the components found in the cad model
	bool			_fuseVertices;							//!< Fuse vertices which are too close
	std::string		_textureFolder;							//!< Folder where model textures may be located
	bool			_useBinary;								//!< Use binary file instead of original obj models

protected:
	/**
	*	@brief Computes a triangle mesh buffer composed only by indices.
	*/
	void computeMeshData(Model3D::ModelComponent* modelComp, bool computeNormals = false);

	/**
	*	@brief Creates a new material from the attributes of a model component.
	*/
	Material* createMaterial(ModelComponent* modelComp);

	/**
	*	@brief Initializes a model component with the content of a mesh.
	*/
	void createModelComponent(objl::Mesh* mesh);

	/**
	*	@brief Fuse components into a single one.
	*/
	void fuseComponents();

	/**
	*	@brief Combines similar vertices.
	*/
	void fuseVertices(std::vector<int>& mapping);

	/**
	*	@brief Fills the content of model component with binary file data.
	*/
	bool loadModelFromBinaryFile();

	/**
	*	@brief Generates geometry via GPU.
	*/
	bool loadModelFromOBJ(const mat4& modelMatrix);

	/**
	*	@brief Loads the CAD model from a binary file, if possible.
	*/
	bool readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp);

	/**
	*	@brief Remaps vertices to merge them.
	*/
	void remapVertices(Model3D::ModelComponent* modelComponent, std::vector<int>& mapping);

	/**
	*	@brief Writes the model to a binary file in order to fasten the following executions.
	*	@return Success of writing process.
	*/
	bool writeToBinary();

public:
	/**
	*	@brief CADModel constructor.
	*	@param filename Path where the model is located.
	*	@param useBinary Use of previously written binary files.
	*/
	CADModel(const std::string& filename, const std::string& textureFolder, const bool useBinary, const bool fuseComponents, const bool fuseVertices);

	/**
	*	@brief Model 3D constructor for a triangle mesh.
	*/
	CADModel(const std::vector<Triangle3D>& triangles, bool releaseMemory = true, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Model 3D constructor for a triangle mesh.
	*/
	CADModel(vec4* vertices, unsigned numVertices, uvec4* faces, unsigned numFaces, bool releaseMemory = true, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted copy constructor.
	*	@param model Model to copy attributes.
	*/
	CADModel(const CADModel& model) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~CADModel();

	/**
	*	@return Bounding box of the model.
	*/
	AABB getAABB() { return _aabb; }

	/**
	*	@brief Loads the model data from file.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted assignment operator.
	*	@param model Model to copy attributes.
	*/
	CADModel& operator=(const CADModel& model) = delete;

	/**
	*	@brief Reloads VAO content.
	*/
	void reload();

	/**
	*	@brief Samples the mesh as a set of points.
	*/
	PointCloud3D* sample(unsigned maxSamples, int randomFunction);

	/**
	*	@brief 
	*/
	void simplify(unsigned numFaces);

	/**
	*	@brief Subdivides mesh with the specified maximum area.
	*/
	bool subdivide(float maxArea);
};

