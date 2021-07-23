#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable

layout(local_size_variable) in;

layout(std430, binding = 0) buffer GridBuffer		{ uint16_t	grid[]; };
layout(std430, binding = 1) buffer NewGridBuffer	{ uint16_t	newGrid[]; };
layout(std430, binding = 2) buffer Stack01Buffer	{ uint		stack01[]; };
layout(std430, binding = 3) buffer Stack02Buffer	{ uint		stack02[]; };
layout(std430, binding = 4) buffer StackSize		{ uint		stackCounter; };
layout(std430, binding = 5) buffer NeighBuffer		{ ivec4		neighborOffset[]; };

#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Fracturer/distance.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform uint	numNeighbors;
uniform uint	stackSize;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= stackSize * numNeighbors) return;

	uint neighborOffsetIdx = index % numNeighbors;
	uint stackIdx = uint(floor(index / numNeighbors));
	uvec4 gridPos = uvec4(getPosition(stack01[stackIdx]), grid[stack01[stackIdx]]);

	ivec4 neighbor = ivec4(gridPos) + neighborOffset[neighborOffsetIdx];
	uint neighborIdx = getPositionIndex(neighbor.xyz);

	if (grid[neighborIdx] == gridPos.w && newGrid[neighborIdx] == VOXEL_EMPTY)
	{
		bool isOutside = neighbor.x < 0 || neighbor.x >= gridDims.x ||
						 neighbor.y < 0 || neighbor.y >= gridDims.y ||
						 neighbor.z < 0 || neighbor.z >= gridDims.z;

		if (!isOutside)
		{
			newGrid[neighborIdx] = uint16_t(gridPos.w);
			stack02[atomicAdd(stackCounter, 1)] = neighborIdx;
		}
	}
}