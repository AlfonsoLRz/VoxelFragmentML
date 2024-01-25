#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelMask.glsl>

layout (std430, binding = 0) buffer GridBuffer { CellGrid	grid[]; };

uniform uint numCells;
uniform uint position;

subroutine uint16_t unmaskType(uint index);
subroutine uniform unmaskType unmaskUniform;

subroutine(unmaskType)
uint16_t unmaskBit(uint index)
{
	return unmaskedBit(grid[index].value, position);
}

subroutine(unmaskType)
uint16_t unmaskRightMost(uint index)
{
	return unmasked(grid[index].value, position);
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells) return;

	grid[index].value = unmaskUniform(index);
}