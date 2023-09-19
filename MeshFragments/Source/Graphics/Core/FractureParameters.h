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
	enum FractureAlgorithm : uint8_t { NAIVE, FLOOD, BASE_ALGORITHMS };
	inline static const char* Fracture_STR[BASE_ALGORITHMS] = { "Naive", "Flood" };

	enum DistanceFunction : uint8_t { EUCLIDEAN, MANHATTAN, CHEBYSHEV, DISTANCE_FUNCTIONS };
	inline static const char* Distance_STR[DISTANCE_FUNCTIONS] = { "Euclidean", "Manhattan", "Chebyshev" };

	enum RandomUniformType { STD_UNIFORM, HALTON, NUM_RANDOM_FUNCTIONS };
	inline static const char* Random_STR[NUM_RANDOM_FUNCTIONS] = { "STD Uniform", "Halton" };

	enum ErosionType { SQUARE, ELLIPSE, CROSS, NUM_EROSION_CONVOLUTIONS };
	inline static const char* Erosion_STR[NUM_EROSION_CONVOLUTIONS] = { "Square", "Ellipse", "Cross" };

public:
	int		_biasSeeds;
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
	ivec3	_gridSubdivisions;
	bool	_launchGPU;
	int		_mergeSeedsDistanceFunction;
	int		_numSeeds;
	int		_numExtraSeeds;
	bool	_removeIsolatedRegions;
	int		_seed;
	int		_seedingRandom;
	int		_spreading;

public:
	/**
	*	@brief Default constructor.
	*/
	FractureParameters() :
		_biasSeeds(1),
		_computeMCFragments(false),
		_erode(true),
		_erosionConvolution(ELLIPSE),
		_erosionProbability(.5f),
		_erosionIterations(3),
		_erosionSize(3),
		_erosionThreshold(0.5f),
		_fillShape(false),
		_fractureAlgorithm(NAIVE),
		_distanceFunction(MANHATTAN),
		_gridSubdivisions(126, 81, 61),
		_launchGPU(true),
		_mergeSeedsDistanceFunction(MANHATTAN),
		_numSeeds(64),
		_numExtraSeeds(200),
		_removeIsolatedRegions(true),
		_seed(80),
		_seedingRandom(STD_UNIFORM),
		_spreading(5)
	{
	}
};

