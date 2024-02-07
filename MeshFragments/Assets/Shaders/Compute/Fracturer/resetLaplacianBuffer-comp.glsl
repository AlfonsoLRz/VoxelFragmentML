#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable

layout(local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/laplacian.glsl>

uniform uint numVertices;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVertices)
		return;

	laplacian01[index] = laplacian02[index] = laplacian03[index] = laplacian04[index] = 0;
}