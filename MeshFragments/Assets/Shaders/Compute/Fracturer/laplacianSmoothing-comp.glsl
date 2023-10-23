#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

layout (std430, binding = 0) buffer VertexBuffer	{ vec4	vertices[]; };
layout (std430, binding = 1) buffer FaceBuffer		{ uvec4 face[]; };
layout (std430, binding = 2) buffer LaplacianBuffer { ivec4 laplacian[]; };

#include <Assets/Shaders/Compute/Templates/constraints.glsl>

uniform uint numFaces;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numFaces)
		return;

	for (int i = 0; i < 3; ++i)
	{
		ivec4 vertex = ivec4(ivec3(vertices[face[index][i]].xyz * UINT_MULT), 1);

		for (int j = 0; j < 4; ++j)
		{
			atomicAdd(laplacian[face[index][(i + 1) % 3]][j], vertex[j]);
			atomicAdd(laplacian[face[index][(i + 2) % 3]][j], vertex[j]);
		}
	}
}