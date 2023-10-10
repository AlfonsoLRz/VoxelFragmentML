#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/constraints.glsl>

layout (std430, binding = 0) buffer IndicesBuffer	{ uint			indices[]; };
layout (std430, binding = 1) buffer OutputBuffer	{ uint			indices2[]; };
layout (std430, binding = 2) buffer PointBuffer		{ vec4			points[]; };
layout (std430, binding = 3) buffer VertexBuffer	{ VertexGPUData	vertexData[]; };
layout (std430, binding = 4) buffer VertexCount		{ uint			vertexCount; };

uniform uint numPoints;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numPoints) return;

	int currentIndex = int(index) - 1;
	vec3 currentPoint = points[indices[index]].xyz;

	while (index >= 0 && distance(currentPoint, points[indices[currentIndex]].xyz) < EPSILON)
	{
		--currentIndex;
	}

	currentIndex += 1;
	if (index != currentIndex)
		indices2[index] = currentIndex;
	else
	{
		uint vertexIndex = atomicAdd(vertexCount, 1);
		vertexData[vertexIndex].position = currentPoint;
		indices2[index] = vertexIndex | (1 << 31);
	}
}