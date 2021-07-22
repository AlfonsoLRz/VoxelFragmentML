#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

layout (std430, binding = 0) buffer SeedBuffer	{ uvec4		seed[]; };
layout (std430, binding = 1) buffer GridBuffer	{ uint16_t  grid[]; };

#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Fracturer/distance.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform uint	numSeeds;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= gridDims.x * gridDims.y * gridDims.z) return;

	if (grid[index] == VOXEL_EMPTY) 
	{
		return;
	}

	// Compute coordinates
	float minDistance = UINT_MAX, distance;
	uvec3 position = getPosition(index);

	for (int seedIdx = 0; seedIdx < numSeeds; ++seedIdx)
	{
		distance = distanceUniform(position, seed[seedIdx].xyz);

		if (distance < minDistance)
		{
			minDistance = distance;
			grid[index] = uint16_t(seed[seedIdx].w);
		}
	}
}