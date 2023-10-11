#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/constraints.glsl>

layout (std430, binding = 0) buffer IndicesBuffer	{ uint			indices[]; };
layout (std430, binding = 1) buffer OutputBuffer	{ uint			indices2[]; };
layout (std430, binding = 2) buffer PointBuffer		{ vec4			points[]; };

uniform uint numPoints;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numPoints) return;

	if (indices2[index] > 10)
	{
		indices[index] = 0;
		return;
	}

	//int previousIndex = clamp(index - 1, 0, int(numPoints) - 1);
	//if (index != 0 && distance(points[indices[index]].xyz, points[indices[previousIndex]].xyz) < EPSILON)
	//{
	//	signal = indices2[previousIndex] >> 31;
	//	if (signal == 1)
	//	{
	//		indices2[index] = indices2[previousIndex];
	//	}
	//	else
	//	{
	//		atomicAdd(nonUpdatedCount, 1);
	//	}
	//}
	//else
	//{
	//	uint vertexIndex = atomicAdd(vertexCount, 1);
	//	vertexData[vertexIndex].position = points[indices[index]].xyz;
	//	indices2[index] = vertexIndex | (1 << 31);
	//}

	//int currentIndex = index - 1;
	//while (index >= 0 && distance(points[indices[index]].xyz, points[indices[currentIndex]].xyz) < EPSILON)
	//{
	//	--currentIndex;
	//}

	//currentIndex += 1;
	//indices2[index] = indices2[currentIndex];
}