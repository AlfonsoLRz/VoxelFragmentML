#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable
layout(local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/constraints.glsl>

layout (std430, binding = 0) buffer InputBuffer		{ uint		indices[]; };
layout (std430, binding = 1) buffer OutputBuffer	{ uint		indices2[]; };
layout (std430, binding = 2) buffer PointBuffer		{ vec4		points[]; };
layout (std430, binding = 3) buffer VertexCount		{ uint		vertexCount; };

uniform uint defaultValue;
uniform uint numPoints;

void main()
{
	const int index = int(gl_GlobalInvocationID.x);
	if (index >= numPoints) return;

	if (indices2[index] != defaultValue)
		return;

	int previousIndex = index - 1;
	if (indices2[previousIndex] == defaultValue)
		atomicAdd(vertexCount, 1);
	else
		indices2[index] = indices2[previousIndex];
}