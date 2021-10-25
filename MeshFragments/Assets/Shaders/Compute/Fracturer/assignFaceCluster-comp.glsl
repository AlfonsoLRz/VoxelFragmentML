#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer VertexBuffer	{ VertexGPUData		vertex[]; };
layout (std430, binding = 1) buffer FaceBuffer		{ FaceGPUData		face[]; };
layout (std430, binding = 2) buffer GridBuffer		{ uint16_t			grid[]; };
layout (std430, binding = 3) buffer ClusterBuffer	{ float				clusterIdx[]; };

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

	//vec3 v1	= vertex[face[index].vertices.x].position, v2 = vertex[face[index].vertices.y].position, v3 = vertex[face[index].vertices.z].position;	
	//vec3 point = (face[index].maxPoint + face[index].minPoint) / 2.0f;
	uvec3 gridIndex = getPositionIndex(vertex[index].position);
	uint16_t clusterID = grid[getPositionIndex(gridIndex)];

	//clusterIdx[face[index].vertices.x] = clusterIdx[face[index].vertices.y] = clusterIdx[face[index].vertices.z] = clusterID;
	clusterIdx[index] = clusterID;
}