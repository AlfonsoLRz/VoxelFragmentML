#pragma once

#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
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

/**
*	@brief Model loaded from an OBJ file.
*/
class CADModel : public Model3D
{
protected:
	static std::unordered_map<std::string, std::unique_ptr<Material>> _cadMaterials;
	static std::unordered_map<std::string, std::unique_ptr<Texture>> _cadTextures;

public:
	const static std::string BINARY_EXTENSION;					//!< File extension for binary models
	const static float MODEL_NORMALIZATION_SCALE;				//!< Scale to normalize the model

protected:
	AABB				_aabb;									//!< Boundaries 
	Assimp::Importer	_assimpImporter;						//!< Assimp importer
	Assimp::Exporter	_assimpExporter;						//!< 
	std::string			_filename;								//!< File path (without extension)
	bool				_fuseVertices;							//!< Fuse vertices which are too close
	const aiScene*		_scene;									//!< Scene from assimp library	
	bool				_useBinary;								//!< Use binary file instead of original obj models

protected:
	/**
	*	@brief Computes a triangle mesh buffer composed only by indices.
	*/
	void computeMeshData(Model3D::ModelComponent* component);

	/**
	*	@brief Creates a new material from the attributes of a model component.
	*/
	Material* createMaterial(ModelComponent* modelComp);

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
	bool loadModelFromBinaryFile(const std::string& binaryFile);

	/**
	*	@brief Processes mesh as loaded by Assimp.
	*/
	Model3D::ModelComponent* processMesh(aiMesh* mesh, const aiScene* scene, const std::string& folder);

	/**
	*	@brief Process node from assimp load.
	*/
	void processNode(aiNode* node, const aiScene* scene, const std::string& folder);

	/**
	*	@brief Loads the CAD model from a binary file, if possible.
	*/
	bool readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp);

	/**
	*	@brief Remaps vertices to merge them.
	*/
	void remapVertices(Model3D::ModelComponent* modelComponent, std::vector<int>& mapping);

	/**
	*	@brief Saves current model using assimp.
	*/
	void saveAssimp(const std::string& filename, const std::string& extension);

	/**
	*	@brief Saves current model using the binary writer of C++.
	*/
	void saveBinary(const std::string& filename);

	/**
	*	@brief Saves current model using assimp.
	*/
	void threadedSaveAssimp(aiScene* scene, const std::string& filename, const std::string& extension = "stl");

	/**
	*	@brief Saves current model using the binary writer of C++.
	*/
	void threadedSaveBinary(const std::string& filename);

	/**
	*	@brief Writes the model to a binary file in order to fasten the following executions.
	*	@return Success of writing process.
	*/
	bool writeBinary(const std::string& path);

public:
	/**
	*	@brief CADModel constructor.
	*	@param filename Path where the model is located.
	*	@param useBinary Use of previously written binary files.
	*/
	CADModel(const std::string& filename, const bool useBinary, const bool fuseVertices);

	/**
	*	@brief Model 3D constructor for a triangle mesh.
	*/
	CADModel(const std::vector<Triangle3D>& triangles, bool releaseMemory = true, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Model 3D constructor for a triangle mesh with no known geometry at first.
	*/
	CADModel(const mat4& modelMatrix = mat4(1.0f));

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
	*	@brief Sends geometry & topology to GPU.
	*/
	void endInsertionBatch(bool releaseMemory = true);

	/**
	*	@return Bounding box of the model.
	*/
	AABB getAABB() { return _aabb; }

	/**
	*	@brief Returns the name of the model.
	*/
	std::string getShortName() const;

	/**
	*	@brief Appends new geometry & topology.
	*/
	void insert(vec4* vertices, unsigned numVertices, uvec4* faces, unsigned numFaces, bool updateIndices = true);

	/**
	*	@brief Loads the model data from file.
	*	@return Success of operation.
	*/
	virtual bool load();

	/**
	*	@brief Deleted assignment operator.
	*	@param model Model to copy attributes.
	*/
	CADModel& operator=(const CADModel& model) = delete;

	/**
	*	@brief Samples the mesh as a set of points.
	*/
	PointCloud3D* sample(unsigned maxSamples, int randomFunction);

	/**
	*	@brief Samples the mesh as a set of points.
	*/
	PointCloud3D* sampleCPU(unsigned maxSamples, int randomFunction);

	/**
	*	@brief Saves the model using assimp.
	*/
	void save(const std::string& filename, FractureParameters::ExportMeshExtension meshExtension);

	/**
	*	@brief
	*/
	void simplify(unsigned numFaces, bool verbose = false);

	/**
	*	@brief Subdivides mesh with the specified maximum area.
	*/
	bool subdivide(float maxArea);

	/**
	*	@brief Transforms the vertices of the model.
	*/
	void transformGeometry(const mat4& mMatrix);
};

