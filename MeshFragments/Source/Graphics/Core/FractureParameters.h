#pragma once

#include "stdafx.h"

/**
*	@file FractureParameters.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
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

public:
	int		_biasSeeds;
	int		_boundarySize;
	bool	_computeMCFragments;
	bool	_erode;
	int		_erosionConvolution;
	int		_erosionIterations;
	float	_erosionProbability;
	int		_erosionSize;
	float	_erosionThreshold;
	bool	_fillShape;
	int		_fractureAlgorithm;
	int		_distanceFunction;
	int		_gridSubdivisions;
	bool	_launchGPU;
	int		_marchingCubesSubdivisions;
	int		_mergeSeedsDistanceFunction;
	int		_numExtraSeeds;
	int		_numSeeds;
	int		_numTriangleSamples;
	int		_pointCloudSeedingRandom;
	bool	_removeIsolatedRegions;
	int		_seed;
	int		_seedingRandom;
	int		_spreading;
	int		_targetTriangles;

	// Rendering during the build procedure
	bool	_renderGrid;
	bool    _renderMesh;
	bool    _renderPointCloud;

public:
	/**
	*	@brief Default constructor.
	*/
	FractureParameters() :
		_biasSeeds(0),
		_boundarySize(1),
		_computeMCFragments(false),
		_erode(false),
		_erosionConvolution(ELLIPSE),
		_erosionProbability(.5f),
		_erosionIterations(3),
		_erosionSize(3),
		_erosionThreshold(0.5f),
		_fillShape(true),
		_fractureAlgorithm(FLOOD),
		_distanceFunction(CHEBYSHEV),
		_gridSubdivisions(256),
		_launchGPU(true),
		_marchingCubesSubdivisions(1),
		_mergeSeedsDistanceFunction(EUCLIDEAN),
		_numExtraSeeds(20),
		_numSeeds(2),
		_numTriangleSamples(10000),
		_pointCloudSeedingRandom(STD_UNIFORM),
		_removeIsolatedRegions(true),
		_seed(80),
		_seedingRandom(STD_UNIFORM),
		_spreading(5),
		_targetTriangles(500),

		_renderGrid(true),
		_renderMesh(true),
		_renderPointCloud(false)
	{
	}
};

