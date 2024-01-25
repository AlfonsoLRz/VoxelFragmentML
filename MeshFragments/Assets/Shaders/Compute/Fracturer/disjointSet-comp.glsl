#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid	grid[]; };
layout (std430, binding = 1) buffer DisjointSet		{ uint		disjointSet[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelMask.glsl>

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= (gridDims.x * gridDims.y * gridDims.z) || grid[index].value <= VOXEL_FREE)
		return;

	atomicMin(disjointSet[unmasked(grid[index].value, MASK_ID_POSITION)], uint(grid[index].value >> MASK_ID_POSITION));
}