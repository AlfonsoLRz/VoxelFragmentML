#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

layout (std430, binding = 0) buffer PointBuffer { vec4		points[]; };
layout (std430, binding = 1) buffer FaceBuffer	{ uvec4		faceData[]; };

uniform uint	numFaces;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numFaces) return;

	faceData[index].w = uint(max(points[faceData[index].x].w, max(points[faceData[index].y].w, points[faceData[index].z].w)));
}