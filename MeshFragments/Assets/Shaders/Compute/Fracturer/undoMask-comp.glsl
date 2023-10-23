#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelMask.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid		grid[]; };

uniform uint numCells;
uniform uint position;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells) return;

	grid[index].value = unmasked(grid[index].value);
}