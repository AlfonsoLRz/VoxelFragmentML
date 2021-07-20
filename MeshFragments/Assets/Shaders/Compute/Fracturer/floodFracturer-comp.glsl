#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

layout (std430, binding = 0) buffer GridBuffer	{ uint16_t  grid[]; };
layout (std430, binding = 1) buffer StackBuffer { uint		stack[]; };
layout (std430, binding = 2) buffer StackSize	{ uint		stackCounter; };
layout (std430, binding = 3) buffer NeighBuffer { ivec4		neighborOffset[]; };

#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Fracturer/distance.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform uvec3	gridDims;
uniform uint	numNeighbors;
uniform uint	stackSize;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= stackSize * numNeighbors) return;

	uint neighborOffsetIdx = index % numNeighbors;
}