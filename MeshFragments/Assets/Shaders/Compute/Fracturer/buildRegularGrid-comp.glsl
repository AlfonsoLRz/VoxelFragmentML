#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer VertexBuffer	{ VertexGPUData		vertex[]; };
layout (std430, binding = 1) buffer FaceBuffer		{ FaceGPUData		face[]; };
layout (std430, binding = 2) buffer NoiseBuffer		{ float				noise[]; };
layout (std430, binding = 3) buffer GridBuffer		{ uint16_t			grid[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform vec3 aabbMin;
uniform vec3 cellSize;
uniform uint numFaces;
uniform uint numSamples;

uvec3 getPositionIndex(vec3 position)
{
	uint x = uint(floor((position.x - aabbMin.x) / cellSize.x)), y = uint(floor((position.y - aabbMin.y) / cellSize.y)), z = uint(floor((position.z - aabbMin.z) / cellSize.z));
	uint zeroUnsigned = 0;

	return uvec3(clamp(x, zeroUnsigned, gridDims.x - 1), clamp(y, zeroUnsigned, gridDims.y - 1), clamp(z, zeroUnsigned, gridDims.z - 1));
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numFaces * numSamples) return;

	uint sampleIdx		= index % numSamples;
	uint faceIdx		= uint(floor(index / numSamples));
	vec3 v1				= vertex[face[faceIdx].vertices.x].position, v2 = vertex[face[faceIdx].vertices.y].position, v3 = vertex[face[faceIdx].vertices.z].position;
	vec3 u				= v2 - v1, v = v3 - v1;
	vec2 randomFactors	= vec2(noise[sampleIdx * 2 + 0], noise[sampleIdx * 2 + 1]);

	if (randomFactors.x + randomFactors.y >= 1.0f)
	{
		randomFactors = 1.0f - randomFactors;
	}

	vec3 point		= v1 + u * randomFactors.x + v * randomFactors.y;
	uvec3 gridIndex = getPositionIndex(point);
	grid[getPositionIndex(gridIndex)] = VOXEL_FREE;
}