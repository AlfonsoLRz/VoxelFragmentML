#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable

layout(local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/laplacian.glsl>
layout (std430, binding = 4) buffer VertexBuffer	{ vec4	vertices[]; };

#include <Assets/Shaders/Compute/Templates/constraints.glsl>

uniform uint	numVertices;
uniform float	targetVertexType;
uniform float	weight;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVertices)
		return;

	if (epsilonEqual(vertices[index].w, targetVertexType) && laplacian04[index] > 0)
	{
		float x = laplacian01[index] / float(laplacian04[index]) / UINT_MULT;
		float y = laplacian02[index] / float(laplacian04[index]) / UINT_MULT;
		float z = laplacian03[index] / float(laplacian04[index]) / UINT_MULT;
		vertices[index].xyz = mix(vertices[index].xyz, vec3(x, y, z), weight);
	}
}