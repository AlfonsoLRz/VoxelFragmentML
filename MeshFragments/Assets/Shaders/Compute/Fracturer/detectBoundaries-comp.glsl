#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ CellGrid		grid[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelMask.glsl>

uniform int		boundarySize;
uniform uint	numCells;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCells || grid[index].value <= VOXEL_FREE) return;

	ivec3 gridIndex = ivec3(getPosition(index));
	ivec3 gridIndex_minusOne = clamp(gridIndex - ivec3(boundarySize), ivec3(0), ivec3(gridDims) - ivec3(1));
	ivec3 gridIndex_plusOne = clamp(gridIndex + ivec3(boundarySize), ivec3(0), ivec3(gridDims) - ivec3(1));
	bool boundary = false;
	uint16_t value;

	for (int x = gridIndex_minusOne.x; x <= gridIndex_plusOne.x && !boundary; ++x)
	{
		for (int y = gridIndex_minusOne.y; y <= gridIndex_plusOne.y && !boundary; ++y)
		{
			for (int z = gridIndex_minusOne.z; z <= gridIndex_plusOne.z && !boundary; ++z)
			{
				value = unmasked(grid[getPositionIndex(uvec3(x, y, z))].value);
				boundary = boundary || (value > VOXEL_FREE && value != grid[index].value);
			}
		}
	}

	if (boundary)
		grid[index].value = masked(grid[index].value);
}