#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer VertexBuffer	{ vec4			vertex[]; };
layout (std430, binding = 1) buffer GridBuffer		{ CellGrid		grid[]; };
layout (std430, binding = 2) buffer ClusterBuffer	{ float			clusterIdx[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform vec3 aabbMin;
uniform vec3 cellSize;
uniform uint numVertices;

uvec3 getPositionIndex(vec3 position)
{
	uint x = uint(floor((position.x - aabbMin.x) / cellSize.x)), y = uint(floor((position.y - aabbMin.y) / cellSize.y)), z = uint(floor((position.z - aabbMin.z) / cellSize.z));
	uint zeroUnsigned = 0;

	return uvec3(clamp(x, zeroUnsigned, gridDims.x - 1), clamp(y, zeroUnsigned, gridDims.y - 1), clamp(z, zeroUnsigned, gridDims.z - 1));
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVertices) return;

	uvec3 gridIndex = getPositionIndex(vertex[index].xyz);
	uint16_t clusterID = grid[getPositionIndex(gridIndex)].value;

	clusterIdx[index] = clusterID;
}