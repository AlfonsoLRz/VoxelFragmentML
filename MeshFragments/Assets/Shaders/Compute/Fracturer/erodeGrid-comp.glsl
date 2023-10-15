#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid		grid[]; };
layout (std430, binding = 1) buffer Convolution		{ float			convolution[]; };
layout (std430, binding = 2) buffer NoiseBuffer		{ float			noise[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform float erosionThreshold, erosionProbability;
uniform uint numActivations;
uniform uint numCells;
uniform uint maskSize;
uniform uint maskSize2;
uniform uint noiseBufferSize;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells) return;

	bool isBoundary = bool((grid[index].value >> 15) & uint16_t(1));
	if (grid[index].value > VOXEL_FREE && isBoundary && noise[index % noiseBufferSize] < erosionProbability)
	{
		ivec3 indices = ivec3(getPosition(index));
		ivec3 maxBounds = indices + ivec3(maskSize2);
		ivec3 minBounds = indices - ivec3(maskSize2);
		ivec3 maxBoundsClamped = clamp(maxBounds, ivec3(0), ivec3(gridDims) + ivec3(maskSize2));
		ivec3 minBoundsClamped = clamp(minBounds, ivec3(0), ivec3(gridDims) - ivec3(maskSize2));
		uint count = 0;

		for (int x = minBoundsClamped.x; x <= maxBoundsClamped.x; ++x)
		{
			for (int y = minBoundsClamped.y; y <= maxBoundsClamped.y; ++y)
			{
				for (int z = minBoundsClamped.z; z <= maxBoundsClamped.z; ++z)
				{
					count += uint(uint(grid[getPositionIndex(uvec3(x, y, z))].value == grid[index].value) * convolution[(x - minBounds.x) * maskSize * maskSize + (y - minBounds.y) * maskSize + (z - minBounds.z)]);
				}
			}
		}

		if (count < numActivations * erosionThreshold)
			grid[index].value = VOXEL_EMPTY;
	}
}