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

	enum ExportMeshExtension { OBJ, STL, BINARY_MESH, NUM_EXPORT_MESH_EXTENSIONS };
	inline static const char* ExportMesh_STR[NUM_EXPORT_MESH_EXTENSIONS] = { "obj", "stl", "binm" };

	enum ExportGrid { RLE, QUADSTACK, VOX, UNCOMPRESSED_BINARY, NUM_GRID_EXTENSIONS };
	inline static const char* ExportGrid_STR[NUM_GRID_EXTENSIONS] = { "rle", "qstack", "vox", "bing"};

	enum ExportPointCloudExtension { PLY, XYZ, COMPRESSED_POINT_CLOUD, NUM_POINT_CLOUD_EXTENSIONS };
	inline static const char* ExportPointCloud_STR[NUM_POINT_CLOUD_EXTENSIONS] = { "ply", "xyz", "binp" };
	 
public:
	int				_biasFocus;
	int				_biasSeeds;
	float			_boundaryMCWeight, _boundaryMCIterations;
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
	float 			_nonBoundaryMCWeight, _nonBoundaryMCIterations;
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
	int				_exportGridExtension;
	int				_exportMeshExtension;
	int				_exportPointCloudExtension;

	bool			_exportGrid;
	bool			_exportMesh;
	bool			_exportPointCloud;

public:
	/**
	*	@brief Default constructor.
	*/
	FractureParameters() :
		_biasSeeds(32),
		_boundaryMCIterations(0.015f),
		_boundaryMCWeight(0.43f),
		_clampVoxelMetricUnit(128),
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
		_nonBoundaryMCIterations(0.048),
		_nonBoundaryMCWeight(0.7f),
		_numExtraSeeds(64),
		_numImpacts(0),
		_numSeeds(8),
		_pointCloudSeedingRandom(STD_UNIFORM),
		_removeIsolatedRegions(true),
		_seed(80),
		_seedingRandom(STD_UNIFORM),
		_biasFocus(5),
		_targetPoints({ 4096 }),
		_targetTriangles({ 5000 }),
		_voxelPerMetricUnit(20),
		_voxelizationSize(256),

		_renderGrid(true),
		_renderMesh(true),
		_renderPointCloud(false),

		_exportGridExtension(VOX),
		_exportMeshExtension(STL),
		_exportPointCloudExtension(PLY),

		_exportGrid(false),
		_exportMesh(false),
		_exportPointCloud(false)
	{
		std::qsort(_targetTriangles.data(), _targetTriangles.size(), sizeof(int), [](const void* a, const void* b) {
			return *(int*)b - *(int*)a;
			});
		std::qsort(_targetPoints.data(), _targetPoints.size(), sizeof(int), [](const void* a, const void* b) {
			return *(int*)b - *(int*)a;
			});

		while (_voxelizationSize.x % 4 != 0)
			_voxelizationSize -= ivec3(1);
	}
};

