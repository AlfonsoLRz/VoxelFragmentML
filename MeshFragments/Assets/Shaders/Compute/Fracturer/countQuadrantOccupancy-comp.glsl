#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid		grid[]; };
layout (std430, binding = 1) buffer Counter			{ uint			counter[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform uint numCells, subdivisions;
uniform uvec3 step;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells) return;

	if (grid[index].value >= VOXEL_FREE)
	{
		uvec3 position = getPosition(index);
		uvec3 counterPosition = uvec3(floor(vec3(position) / step));
		uint counterIndex = counterPosition.x * subdivisions * subdivisions + counterPosition.y * subdivisions + counterPosition.z;

		atomicAdd(counter[counterIndex], 1);
	}
}