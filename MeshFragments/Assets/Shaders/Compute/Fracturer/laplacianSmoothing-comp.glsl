#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/laplacian.glsl>
layout (std430, binding = 4) buffer VertexBuffer	{ vec4	vertices[]; };
layout (std430, binding = 5) buffer FaceBuffer		{ uvec4 face[]; };

#include <Assets/Shaders/Compute/Templates/constraints.glsl>

uniform uint	checkValidity;
uniform uint	numFaces;
uniform float	targetVertexType;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numFaces)
		return;

	bool isFaceValid = true;
	if (checkValidity == 1)
	{
		for (int i = 0; i < 3 && isFaceValid; ++i)
			isFaceValid = isFaceValid && epsilonEqual(vertices[face[index][i]].w, targetVertexType);
	}

	for (int i = 0; i < 3 && isFaceValid; ++i)
	{
		ivec4 vertex = ivec4(ivec3(vertices[face[index][i]].xyz * UINT_MULT), 1);
		uint index1 = face[index][(i + 1) % 3], index2 = face[index][(i + 2) % 3];

		atomicAdd(laplacian01[index1], vertex[0]);
		atomicAdd(laplacian01[index2], vertex[0]);

		atomicAdd(laplacian02[index1], vertex[1]);
		atomicAdd(laplacian02[index2], vertex[1]);

		atomicAdd(laplacian03[index1], vertex[2]);
		atomicAdd(laplacian03[index2], vertex[2]);

		atomicAdd(laplacian04[index1], vertex[3]);
		atomicAdd(laplacian04[index2], vertex[3]);
	}
}