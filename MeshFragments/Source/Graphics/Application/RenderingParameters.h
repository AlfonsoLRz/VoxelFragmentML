#pragma once

#include "Graphics/Application/GraphicsAppEnumerations.h"

/**
*	@file RenderingParameters.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/20/2019
*/

/**
*	@brief Wraps the rendering conditions of the application.
*/
struct RenderingParameters
{
public:
	enum PointCloudRendering : int { ORIGINAL_MESH_PC, SAMPLED_POINT_CLOUD, FRAGMENTED_MESH_PC, NUM_POINT_CLOUD_RENDERING_TYPES };
	inline static const char* PointCloudRendering_STR[NUM_POINT_CLOUD_RENDERING_TYPES] = { "Starting mesh", "Sampled point cloud", "Fragmented mesh" };

	enum WireframeRendering : int { ORIGINAL_MESH_W, FRAGMENTED_MESH_W, NUM_WIREFRAME_RENDERING_TYPES };
	inline static const char* WireframeRendering_STR[NUM_WIREFRAME_RENDERING_TYPES] = { "Starting mesh", "Fragmented mesh" };

	enum TriangleMeshRendering : int { ORIGINAL_MESH_T, VOXELIZATION, FRAGMENTED_MESH_T, CLUSTERED_MESH, NUM_TRIANGLE_MESH_RENDERING_TYPES };
	inline static const char* TriangleMeshRendering_STR[NUM_TRIANGLE_MESH_RENDERING_TYPES] = { "Starting mesh", "Voxelization", "Fragmented mesh", "Clustered mesh" };

public:
	// Application
	vec3							_backgroundColor;						//!< Clear color
	ivec2							_viewportSize;							//!< Viewport size (!= window)

	// Lighting
	float							_materialScattering;					//!< Ambient light substitute
	float							_occlusionMinIntensity;					//!< Mininum value for occlusion factor (max is 1 => no occlusion)

	// Screenshot
	char							_screenshotFilenameBuffer[32];			//!< Location of screenshot
	float							_screenshotMultiplier;					//!< Multiplier of current size of GLFW window

	// Rendering type
	int								_visualizationMode;						//!< Only triangle mesh is defined here
	
	// Point cloud	
	float							_scenePointSize;						//!< Size of points in a cloud
	vec3							_scenePointCloudColor;					//!< Color of point cloud which shows all the vertices
	bool							_useUniformPointColor;					//!< 
	bool							_useClusterColor;						//!<

	// Wireframe
	bool							_showVertexNormal;						//!< Normals rendered through a geometry shader
	vec3							_wireframeColor;						//!< Color of lines in wireframe rendering

	// Triangle mesh
	bool							_ambientOcclusion;						//!< Boolean value to enable/disable occlusion

	// What to see		
	bool							_planeClipping;							//!< 
	vec4							_planeCoefficients;						//!< 
	int								_pointCloudRendering;					//!< ID of the point cloud type which must be rendered
	int								_wireframeRendering;					//!<
	int								_triangleMeshRendering;					//!<

public:
	/**
	*	@brief Default constructor.
	*/
	RenderingParameters() :
		_viewportSize(1.0f, 1.0f),

		_backgroundColor(0.4f, 0.4f, 0.4f),

		_materialScattering(1.0f),

		_screenshotFilenameBuffer("Screenshot.png"),
		_screenshotMultiplier(3.0f),

		_visualizationMode(CGAppEnum::VIS_TRIANGLES),

		_scenePointSize(2.0f),
		_scenePointCloudColor(1.0f, .0f, .0f),
		_useUniformPointColor(true),
		_useClusterColor(true),

		_showVertexNormal(false),
		_wireframeColor(0.0f),

		_ambientOcclusion(true),

		_planeClipping(false),
		_planeCoefficients(.0f),
		_pointCloudRendering(ORIGINAL_MESH_PC),
		_wireframeRendering(ORIGINAL_MESH_W),
		_triangleMeshRendering(VOXELIZATION)
	{
	}
};

