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

public:
	int		_fractureAlgorithm;
	int		_distanceFunction;
	ivec3	_gridSubdivisions;
	bool	_launchGPU;
	int		_numSeeds;
	int		_numExtraSeeds;
	bool	_removeIsolatedRegions;
	bool	_useExtraSeeds;

public:
	/**
	*	@brief Default constructor.
	*/
	FractureParameters() :
		_fractureAlgorithm(NAIVE),
		_distanceFunction(MANHATTAN),
		_gridSubdivisions(126, 81, 61),
		_launchGPU(true),
		_numSeeds(64),
		_numExtraSeeds(200),
		_removeIsolatedRegions(true),
		_useExtraSeeds(false)
	{
	}
};

