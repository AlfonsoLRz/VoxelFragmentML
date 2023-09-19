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
	enum PointCloudType : int {
		UNIFORM = 0,
		HEIGHT = 1
	};

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

	// Wireframe
	bool							_showVertexNormal;						//!< Normals rendered through a geometry shader
	vec3							_wireframeColor;						//!< Color of lines in wireframe rendering

	// Triangle mesh
	bool							_ambientOcclusion;						//!< Boolean value to enable/disable occlusion

	// What to see		
	bool							_planeClipping;							//!< 
	vec4							_planeCoefficients;						//!< 
	int								_pointCloudType;						//!< ID of the point cloud type which must be rendered
	bool							_showFragmentsMarchingCubes;			//!<
	bool							_showVoxelizedMesh;						//!< Renders mesh as a voxelized model
	bool							_showTriangleMesh;						//!< Render original scene

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

		_showVertexNormal(false),
		_wireframeColor(0.0f),

		_ambientOcclusion(true),

		_planeClipping(false),
		_planeCoefficients(.0f),
		_pointCloudType(PointCloudType::UNIFORM),
		_showFragmentsMarchingCubes(false),
		_showVoxelizedMesh(true),
		_showTriangleMesh(true)
	{
	}
};

