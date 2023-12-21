#pragma once

#include "stdafx.h"

/**
*	@file FractureParameters.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 21/07/2021
*/

/**
*	@brief Wraps the rendering conditions of the application.
*/
struct FractureParameters
{
public:
	// --------- Base algorithm ---------
	enum FractureAlgorithm : uint8_t { NAIVE, FLOOD, VORONOI, BASE_ALGORITHMS };
	inline static const char* Fracture_STR[BASE_ALGORITHMS] = { "Naive", "Flood", "Voronoi" };

	enum DistanceFunction : uint8_t { EUCLIDEAN, MANHATTAN, CHEBYSHEV, DISTANCE_FUNCTIONS };
	inline static const char* Distance_STR[DISTANCE_FUNCTIONS] = { "Euclidean", "Manhattan", "Chebyshev" };

	enum RandomUniformType { STD_UNIFORM, HALTON, NUM_RANDOM_FUNCTIONS };
	inline static const char* Random_STR[NUM_RANDOM_FUNCTIONS] = { "STD Uniform", "Halton" };

	enum ErosionType { SQUARE, ELLIPSE, CROSS, NUM_EROSION_CONVOLUTIONS };
	inline static const char* Erosion_STR[NUM_EROSION_CONVOLUTIONS] = { "Square", "Ellipse", "Cross" };

	enum NeighbourhoodType { VON_NEUMANN, MOORE, NUM_NEIGHBOURHOODS };
	inline static const char* Neighbourhood_STR[NUM_NEIGHBOURHOODS] = { "Von Neumann", "Moore" };

	enum ExportMeshExtension { OBJ, PLY, STL, NUM_EXPORT_MESH_EXTENSIONS };
	inline static const char* ExportMesh_STR[NUM_EXPORT_MESH_EXTENSIONS] = { ".obj", ".ply", ".stl"};
	 
public:
	int				_biasFocus;
	int				_biasSeeds;
	int				_boundarySize;
	int				_clampVoxelMetricUnit;
	bool			_erode;
	int				_erosionConvolution;
	int				_erosionIterations;
	float			_erosionProbability;
	int				_erosionSize;
	float			_erosionThreshold;
	int				_fractureAlgorithm;
	int				_distanceFunction;
	bool			_launchGPU;
	int				_marchingCubesSubdivisions;
	int				_mergeSeedsDistanceFunction;
	bool			_metricVoxelization;
	int             _neighbourhoodType;
	int				_numExtraSeeds;
	int				_numImpacts;
	int				_numSeeds;
	int				_pointCloudSeedingRandom;
	bool			_removeIsolatedRegions;
	int				_seed;
	int				_seedingRandom;
	std::vector<int> _targetPoints;
	std::vector<int> _targetTriangles;
	int				_voxelPerMetricUnit;
	ivec3			_voxelizationSize;

	// Rendering during the build procedure
	bool			_renderGrid;
	bool			_renderMesh;
	bool			_renderPointCloud;

	// Export		
	int				_exportMeshExtension;
	bool			_exportProcessedMesh;
	bool			_exportStartingGrid;

public:
	/**
	*	@brief Default constructor.
	*/
	FractureParameters() :
		_biasSeeds(32),
		_boundarySize(1),
		_clampVoxelMetricUnit(512),
		_erode(false),
		_erosionConvolution(ELLIPSE),
		_erosionProbability(.5f),
		_erosionIterations(3),
		_erosionSize(3),
		_erosionThreshold(0.5f),
		_fractureAlgorithm(FLOOD),
		_distanceFunction(CHEBYSHEV),
		_launchGPU(true),
		_marchingCubesSubdivisions(1),
		_mergeSeedsDistanceFunction(EUCLIDEAN),
		_metricVoxelization(false),
		_neighbourhoodType(VON_NEUMANN),
		_numExtraSeeds(64),
		_numImpacts(0),
		_numSeeds(8),
		_pointCloudSeedingRandom(HALTON),
		_removeIsolatedRegions(true),
		_seed(80),
		_seedingRandom(HALTON),
		_biasFocus(5),
		_targetPoints({ 500, 1000 }),
		_targetTriangles({ 500, 1000 }),
		_voxelPerMetricUnit(90),
		_voxelizationSize(256),

		_renderGrid(true),
		_renderMesh(true),
		_renderPointCloud(false),

		_exportMeshExtension(OBJ),
		_exportProcessedMesh(false),
		_exportStartingGrid(false)
	{
		std::qsort(_targetTriangles.data(), _targetTriangles.size(), sizeof(int), [](const void* a, const void* b) {
			return *(int*)b - *(int*)a;
			});
		std::qsort(_targetPoints.data(), _targetPoints.size(), sizeof(int), [](const void* a, const void* b) {
			return *(int*)b - *(int*)a;
			});
	}
};

