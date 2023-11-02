#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer	{ CellGrid	grid[]; };
layout (std430, binding = 1) buffer Counter		{ uint		counter[]; };

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

uniform uint directionIdx;
uniform uint numVoxels;

bool areNeighbours(ivec3 p1, ivec3 p2)
{
	return distance(p1, p2) <= (1.0f + EPSILON);
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVoxels || grid[index].value >= VOXEL_FREE || counter[index] < directionIdx)
		return;

	uint positionIndex, count = 0;
	ivec3 lastPosition = ivec3(-100);
	ivec3 position = ivec3(getPosition(index)) + direction[directionIdx];
	{
		while (position.x >= 0 && position.y >= 0 && position.z >= 0 && position.x < gridDims.x && position.y < gridDims.y && position.z < gridDims.z)
		{
			positionIndex = getPositionIndex(position);
			if (grid[positionIndex].value >= VOXEL_FREE)
			{
				if (!areNeighbours(position, lastPosition))
				{
					lastPosition = position;
					count += 1;
				}
			}

			position += direction[directionIdx];
		}
	}

	if (count > 0 && count % 2 == 1)
		counter[index] += 1;
}