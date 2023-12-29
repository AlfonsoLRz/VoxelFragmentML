#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid		grid[]; };
layout (std430, binding = 1) buffer GridBuffer2		{ CellGrid		destGrid[]; };

uniform uint numCells;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells) return;

	grid[index] = destGrid[index];
}