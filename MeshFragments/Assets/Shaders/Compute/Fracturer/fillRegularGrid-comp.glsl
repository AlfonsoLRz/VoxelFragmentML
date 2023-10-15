#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer	{ CellGrid	grid[]; };
layout (std430, binding = 1) buffer Counter		{ uint		counter; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

const ivec3 direction[6] =
{
	ivec3(0, 0, 1),
	ivec3(0, 0, -1),
	ivec3(-1, 0, 0),
	ivec3(1, 0, 0),
	ivec3(0, 1, 0),
	ivec3(0, -1, 0)
};

uniform uint numVoxels;


void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVoxels)
	{
		return;
	}

	if (grid[index].value == VOXEL_FREE)
		return;

	ivec3 startingPosition = ivec3(getPosition(index));
	uint positionIndex, count = 0;

	for (int directionIdx = 0; directionIdx < 6 && count == directionIdx; ++directionIdx)
	{
		ivec3 position = startingPosition;

		while (position.x >= 0 && position.y >= 0 && position.z >= 0 && position.x < gridDims.x && position.y < gridDims.y && position.z < gridDims.z)
		{
			positionIndex = getPositionIndex(position);
			if (grid[positionIndex].value == VOXEL_FREE)
			{
				count += 1;
				break;
			}

			position += direction[directionIdx];
		}
	}

	if (count == 6)
	{
		atomicAdd(counter, 1);
		grid[index].value = VOXEL_FREE;
	}
}