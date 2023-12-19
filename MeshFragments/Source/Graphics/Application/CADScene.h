#pragma once

#include "DataStructures/RegularGrid.h"
#include "Fracturer/FloodFracturer.h"
#include "Fracturer/NaiveFracturer.h"
#include "Fracturer/Seeder.h"
#include "Graphics/Application/SSAOScene.h"

class AABBSet;
class DrawLines;
class DrawPointCloud;
class FractureParameters;
class FragmentationProcedure;
class PointCloud3D;


#define NEW_LIGHT "!"

#define CAMERA_POS_HEADER "Position"
#define CAMERA_LOOKAT_HEADER "LookAt"

#define AMBIENT_INTENSITY "AmbientIntensity"
#define DIFFUSE_INTENSITY "DiffuseIntensity"
#define SPECULAR_INTENSITY "SpecularIntensity"
#define LIGHT_TYPE "LightType"
#define ORTHO_SIZE "OrthoBottomLeftCorner"
#define LIGHT_POSITION "LightPosition"
#define LIGHT_DIRECTION "LightDirection"
#define CAST_SHADOWS "CastShadows"
#define SHADOW_INTENSITY "ShadowIntensity"
#define SHADOW_MAP_SIZE "ShadowMapSize"
#define BLUR_SHADOW_SIZE "BlurShadow"
#define SHADOW_CAMERA_ANGLE_X "ShadowCameraAngle_X"
#define SHADOW_CAMERA_ANGLE_Y "ShadowCameraAngle_Y"
#define SHADOW_CAMERA_RASPECT "ShadowCameraRaspect"
#define SHADOW_RADIUS "ShadowRadius"

/**
*	@file CADScene.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 28/11/2020
*/

/**
*	@brief Scene composed of CAD models for the manuscript of this work.
*/
class CADScene : public SSAOScene
{
protected:
	// Meshes to be tested
	const static std::string TARGET_PATH;						//!< Location of the default mesh in the file system

protected:
	AABBSet*					_aabbRenderer;					//!< Buffer of voxels
	DrawLines*					_fragmentBoundaries;			//!<
	FractureParameters			_fractParameters;				//!< 
	std::vector<Model3D*>		_fractureMeshes;				//!<
	std::vector<Material*>		_fragmentMaterials;				//!< Material for each fragment, built with marching cubes
	FragmentMetadataBuffer		_fragmentMetadata;				//!< Metadata of the current fragmentation procedure
	std::vector<Texture*>		_fragmentTextures;				//!< Texture for each fragment, built with marching cubes
	bool						_generateDataset;
	std::vector<uvec4>			_impactSeeds;					//!< Seeds obtained by impacting the user's ray to the voxelization
	CADModel*					_mesh;							//!< Jar mesh
	RegularGrid*				_meshGrid;						//!< Mesh regular grid
	PointCloud3D*				_pointCloud;					//!<
	DrawPointCloud*				_pointCloudRenderer;			//!<

protected:
	/**
	*	@brief Allocates memory for the scene.
	*/
	void allocateMemoryDataset(FragmentationProcedure& fractureProcedure);

	/**
	*	@brief Allocates the mesh grid.
	*/
	void allocateMeshGrid(FractureParameters& fractParameters);

	/**
	*	@brief Erase content from a previous fragmentation process.
	*/
	void eraseFragmentContent();

	/**
	*	@brief
	*/
	void exportMetadata(const std::string& filename, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentSize);

	/**
	*	@brief Splits the loaded mesh into fragments through a fracturer algorithm.
	*/
	std::string fractureModel(FractureParameters& fractParameters);

	/**
	*	@brief Saves the whole folder into another one.
	*/
	void launchZipingProcess(const std::string& folder);

	/**
	*	@brief Loads a camera with code-defined values.
	*/
	void loadDefaultCamera(Camera* camera);

	/**
	*	@brief Loads lights with code-defined values.
	*/
	void loadDefaultLights();

	/**
	*	@brief Loads the cameras from where the scene is observed. If no file is accessible, then a default camera is loaded.
	*/
	virtual void loadCameras();

	/**
	*	@brief Loads the lights which affects the scene. If no file is accessible, then default lights are loaded.
	*/
	virtual void loadLights();

	/**
	*	@brief Loads the models which are necessary to render the scene.
	*/
	virtual void loadModels();

	/**
	*	@brief Replaces the currently loaded model.
	*/
	void loadModel(const std::string& path);

	/**
	*	@brief Updates the scene content.
	*/
	void prepareScene(FractureParameters& fractureParameters, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FragmentationProcedure* datasetProcedure = nullptr);

	/**
	*	@brief Rebuilds the whole grid to adapt it to a different number of subdivisions.
	*/
	void rebuildGrid(FractureParameters& fractParameters);

	// ------------- Rendering ----------------

	/**
	*	@brief Renders the scene as a set of triangles.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void drawAsTriangles(Camera* camera, const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as points.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsPoints(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as a wireframe mesh.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsLines(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as a triangle mesh.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsTriangles(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as a triangle mesh. Only the normal is calculated for each fragment.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsTriangles4Normal(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as a triangle mesh. Only the position is calculated for each fragment.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsTriangles4Position(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

public:
	/**
	*	@brief Default constructor.
	*/
	CADScene();

	/**
	*	@brief Destructor. Frees memory allocated for 3d models.
	*/
	virtual ~CADScene();

	/**
	*	@brief
	*/
	void exportGrid();

	/**
	*	@brief Exports fragments into several models in a given extension.
	*/
	void exportFragments(const FractureParameters& fractureParameters, const std::string& extension = ".obj");

	/**
	*	@brief Fractures voxelized model.
	*/
	std::string fractureGrid(std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters);

	/**
	*	@brief Fractures voxelized model.
	*/
	std::string fractureGrid(const std::string& path, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters);

	/**
	*	@brief Generates a dataset of fractured models.
	*/
	void generateDataset(FragmentationProcedure& fractureProcedure, const std::string& folder, const std::string& extension, const std::string& destinationFolder);

	/**
	*	@return Buffer with information of split fragments.
	*/
	std::vector<FragmentationProcedure::FragmentMetadata> getFragmentMetadata() { return _fragmentMetadata; }

	/**
	*	@brief Hits the scene and creates a new fragmentation pattern.
	*/
	void hit(const Model3D::RayGPUData& ray);

	/**
	*	@brief Draws the scene as the rendering parameters specifies.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void render(const mat4& mModel, RenderingParameters* rendParams);

	// ----- Getters

	/**
	*	@return Parameters for fracturing a model.
	*/
	FractureParameters* getFractureParameters() { return &_fractParameters; }
};

