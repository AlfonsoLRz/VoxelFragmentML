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

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVoxels || grid[index].value == VOXEL_FREE || counter[index] < directionIdx)
		return;

	uint positionIndex;
	ivec3 position = ivec3(getPosition(index));
	{
		while (position.x >= 0 && position.y >= 0 && position.z >= 0 && position.x < gridDims.x && position.y < gridDims.y && position.z < gridDims.z)
		{
			positionIndex = getPositionIndex(position);
			if (grid[positionIndex].value >= VOXEL_FREE)
			{
				counter[index] += 1;
				break;
			}

			position += direction[directionIdx];
		}
	}

	if (counter[index] == 6)
		grid[index].value = VOXEL_FREE;
}