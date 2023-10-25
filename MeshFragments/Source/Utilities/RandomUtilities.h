#pragma once

#include "stdafx.h"

/**
*	@file RandomUtilities.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 17/04/2021
*/

typedef std::mt19937							RandomNumberGenerator;
typedef std::uniform_real_distribution<float>	DoubleUniformDistribution;

/**
*	@brief Set of utilities to retrieve random values.
*	@author Alfonso López Ruiz.
*/
class RandomUtilities
{
protected:
	static inline RandomNumberGenerator generator;
	static inline DoubleUniformDistribution distribution = DoubleUniformDistribution(.0f, 1.0f);

public:
	/**
	*	@brief Initializes the seed of the current distribution.
	*/
	static void initSeed(int seed);

	/**
	*	@return Random of length up to distanceSquared.
	*/
	static vec3 getRandomToSphere(float radius, float distanceSquared);

	/**
	*	@return New random value retrieved from a random uniform distribution.
	*/
	static float getUniformRandom();

	/**
	*	@return New random value retrieved from a random uniform distribution. Note that this value is not in [0, 1].
	*/
	static float getUniformRandom(float min, float max);

	/**
	*	@brief Generates a random color in [0, 1] by using getUniformRandom function for each channel.
	*/
	static vec3 getUniformRandomColor();

	/**
	*	@brief Generates a random color by using getUniformRandom function for each channel.
	*/
	static vec3 getUniformRandomColor(float min, float max);

	/**
	*	@return Random hemisphere vector aligned to Z axis.
	*/
	static vec3 getUniformRandomCosineDirection();

	/**
	*	@return Random point in unit sphere.
	*/
	static vec3 getUniformRandomInHemisphere(const vec3& normal);

	/**
	*	@return Random single integer value.
	*/
	static int getUniformRandomInt(int min, int max);

	/**
	*	@return Random single integer value biased towards the middle value.
	*/
	static int getBiasedRandomInt(int min, int max, int divs);

	/**
	*	@return Random point in unit disk.
	*/
	static vec3 getUniformRandomInUnitDisk();

	/**
	*	@return Random point in unit sphere.
	*/
	static vec3 getUniformRandomInUnitSphere();
};

inline void RandomUtilities::initSeed(int seed)
{
	generator = RandomNumberGenerator(seed);
}

inline vec3 RandomUtilities::getRandomToSphere(float radius, float distanceSquared)
{
	const float r1 = getUniformRandom();
	const float r2 = getUniformRandom();
	const float z = 1 + r2 * (sqrt(1.0f - radius * radius / distanceSquared) - 1.0f);
	const float phi = 2.0f * glm::pi<float>() * r1;
	const float x = std::cos(phi) * sqrt(1 - z * z);
	const float y = std::sin(phi) * sqrt(1 - z * z);

	return vec3(x, y, z);
}

inline float RandomUtilities::getUniformRandom()
{
	return distribution(generator);
}

inline float RandomUtilities::getUniformRandom(float min, float max)
{
	return min + (max - min) * getUniformRandom();
}

inline vec3 RandomUtilities::getUniformRandomColor()
{
	return vec3(RandomUtilities::getUniformRandom(), RandomUtilities::getUniformRandom(), RandomUtilities::getUniformRandom());
}

inline vec3 RandomUtilities::getUniformRandomColor(float min, float max)
{
	return vec3(RandomUtilities::getUniformRandom(min, max), RandomUtilities::getUniformRandom(min, max), RandomUtilities::getUniformRandom(min, max));
}

inline vec3 RandomUtilities::getUniformRandomCosineDirection()
{
	const float r1 = RandomUtilities::getUniformRandom(), r2 = RandomUtilities::getUniformRandom();
	const float z = sqrt(1 - r2);
	const float phi = 2.0f * glm::pi<float>() * r1;
	const float x = std::cos(phi) * sqrt(r2);
	const float y = std::sin(phi) * sqrt(r2);

	return vec3(x, y, z);
}

inline vec3 RandomUtilities::getUniformRandomInHemisphere(const vec3& normal)
{
	vec3 unitSphere = getUniformRandomInUnitSphere();

	return unitSphere * -1.0f * ((glm::dot(unitSphere, normal) > .0f) * 2.0f - 1.0f);
}

inline int RandomUtilities::getUniformRandomInt(int min, int max)
{
	return static_cast<int>(getUniformRandom(min, max));
}

inline int RandomUtilities::getBiasedRandomInt(int min, int max, int divs)
{
	int number = 0;
	max /= divs;
	
	for (int i = 0; i < divs; i++) {
		number += rand() % max;
	}

	return number;
}

inline vec3 RandomUtilities::getUniformRandomInUnitDisk()
{
	while (true)
	{
		vec3 point = vec3(getUniformRandom(-1.0f, 1.0f), getUniformRandom(-1.0f, 1.0f), .0f);
		if (glm::length2(point) >= 1) continue;

		return point;
	}
}

inline vec3 RandomUtilities::getUniformRandomInUnitSphere()
{
	vec3 point;
	while (true)
	{
		point = vec3(getUniformRandom(-1.0f, 1.0f), getUniformRandom(-1.0f, 1.0f), getUniformRandom(-1.0f, 1.0f));
		if (glm::length2(point) >= 1) continue;

		return point;
	}
}
