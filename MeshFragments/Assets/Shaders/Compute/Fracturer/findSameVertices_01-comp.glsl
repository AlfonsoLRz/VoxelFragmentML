#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable
layout(local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/constraints.glsl>

layout (std430, binding = 0) buffer IndicesBuffer	{ uint			indices[]; };
layout (std430, binding = 1) buffer OutputBuffer	{ uint			indices2[]; };
layout (std430, binding = 2) buffer PointBuffer		{ vec4			points[]; };
layout (std430, binding = 3) buffer VertexBuffer	{ vec4			vertexData[]; };
layout (std430, binding = 4) buffer VertexCount		{ uint			vertexCount; };

uniform uint defaultValue;
uniform mat4 modelMatrix;
uniform uint numPoints;

void main()
{
	const int index = int(gl_GlobalInvocationID.x);
	if (index >= numPoints) return;

	indices2[index] = defaultValue;

	int previousIndex = clamp(index - 1, 0, int(numPoints) - 1);
	if (index == 0 || distance(points[indices[index]].xyz, points[indices[previousIndex]].xyz) > EPSILON)
	{
		uint vertexIndex = atomicAdd(vertexCount, 1);
		vertexData[vertexIndex] = vec4((modelMatrix * vec4(points[indices[index]].xyz, 1.0f)).xyz, points[indices[index]].w);
		indices2[index] = vertexIndex;
	}
}