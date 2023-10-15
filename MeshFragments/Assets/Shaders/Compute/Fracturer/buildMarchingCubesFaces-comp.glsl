#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer IndicesBuffer	{ uint			indices[]; };
layout (std430, binding = 1) buffer OutputBuffer	{ uint			indices2[]; };
layout (std430, binding = 2) buffer FaceBuffer		{ uvec4			faceData[]; };
layout (std430, binding = 3) buffer FaceCount		{ uint			faceCount; };

uniform uint numPoints;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numPoints) return;
	
	uint faceIndex = indices[index] / 3;
	uint vertexIndex = indices[index] % 3;
	faceData[faceIndex][vertexIndex] = indices2[index];
}