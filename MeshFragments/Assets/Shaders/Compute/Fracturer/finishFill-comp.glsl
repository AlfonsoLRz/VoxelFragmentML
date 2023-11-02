#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;
	
#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer	{ CellGrid	grid[]; };
layout (std430, binding = 1) buffer Counter		{ uint		counter[]; };

uniform uint numNeighbours;
uniform uint numVoxels;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVoxels || grid[index].value >= VOXEL_FREE || counter[index] < numNeighbours)
		return;

	grid[index].value = VOXEL_FREE;
}