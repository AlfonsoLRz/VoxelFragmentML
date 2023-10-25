#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable

layout(local_size_variable) in;

layout (std430, binding = 0) buffer VertexBuffer	{ vec4	vertices[]; };
layout (std430, binding = 1) buffer LaplacianBuffer { ivec4 laplacian[]; };

#include <Assets/Shaders/Compute/Templates/constraints.glsl>

uniform uint	numVertices;
uniform float	weight;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVertices)
		return;

	if (vertices[index].w == 1)
	{
		vertices[index].xyz = mix(vertices[index].xyz, laplacian[index].xyz / float(laplacian[index].w) / UINT_MULT, weight);
	}
	else
		vertices[index].xyz = laplacian[index].xyz / float(laplacian[index].w) / UINT_MULT;
}