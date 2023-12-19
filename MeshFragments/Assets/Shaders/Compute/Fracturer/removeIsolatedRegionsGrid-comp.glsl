#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid		grid[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform uint numCells;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells) return;

	uint count = 0;
	ivec3 indices = ivec3(getPosition(index));
	ivec3 maxBoundsClamped = clamp(indices + ivec3(1), ivec3(0), ivec3(gridDims) - ivec3(1));
	ivec3 minBoundsClamped = clamp(indices - ivec3(1), ivec3(0), ivec3(gridDims) - ivec3(1));

	for (int x = minBoundsClamped.x; x <= maxBoundsClamped.x; ++x)
	{
		for (int y = minBoundsClamped.y; y <= maxBoundsClamped.y; ++y)
		{
			for (int z = minBoundsClamped.z; z <= maxBoundsClamped.z; ++z)
			{
				count += uint(grid[getPositionIndex(uvec3(x, y, z))].value == grid[index].value);
			}
		}
	}

	if (count < 6)
		grid[index].value = VOXEL_EMPTY;
}