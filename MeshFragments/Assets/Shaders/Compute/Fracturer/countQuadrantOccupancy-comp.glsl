#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid		grid[]; };
layout (std430, binding = 1) buffer Counter			{ uint			counter[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform uint numCells;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells || grid[index].value < VOXEL_FREE) return;

	ivec3 position = ivec3(getPosition(index));
	ivec3 minPosition = min(position - ivec3(1), ivec3(0)), maxPosition = max(position + ivec3(1), ivec3(gridDims) - ivec3(1));
	bool allEqual = true;
	uint16_t value = grid[index].value;

	for (uint x = minPosition.x; x <= maxPosition.x && allEqual; x++)
	{
		for (uint y = minPosition.y; y <= maxPosition.y && allEqual; y++)
		{
			for (uint z = minPosition.z; z <= maxPosition.z && allEqual; z++)
			{
				allEqual = allEqual && grid[getPositionIndex(uvec3(x, y, z))].value == value;
			}
		}
	}

	if (!allEqual)
		atomicAdd(counter[0], 1);
}