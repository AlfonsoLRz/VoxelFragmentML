#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid	grid[]; };
layout (std430, binding = 1) buffer DisjointSet		{ uint		disjointSet[]; };
layout (std430, binding = 2) buffer Stack			{ uint		stack[]; };
layout (std430, binding = 3) buffer StackSize		{ uint		stackSize; };
layout (std430, binding = 4) buffer DisjointVoxels	{ uint		disjointVoxels; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelMask.glsl>

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= (gridDims.x * gridDims.y * gridDims.z) || grid[index].value <= VOXEL_FREE)
		return;

	uint seedIdx = grid[index].value >> MASK_ID_POSITION, fragmentIdx = unmasked(grid[index].value, MASK_ID_POSITION);
	if (seedIdx != disjointSet[fragmentIdx])
	{
		grid[index].value = VOXEL_FREE;
		atomicAdd(disjointVoxels, 1);
	}
	else
	{
		stack[atomicAdd(stackSize, 1)] = index;
	}

}