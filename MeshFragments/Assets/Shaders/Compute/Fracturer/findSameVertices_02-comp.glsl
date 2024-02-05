#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable
layout(local_size_variable) in;

layout (std430, binding = 0) buffer InputBuffer		{ uint		indices[]; };
layout (std430, binding = 1) buffer OutputBuffer	{ uint		indices2[]; };

uniform uint defaultValue;
uniform uint numPoints;

void main()
{
	const int index = int(gl_GlobalInvocationID.x);
	if (index >= numPoints) return;

	if (indices2[index] == defaultValue)
		return;

	int nextIndex = index + 1;
	while (nextIndex < numPoints && indices2[nextIndex] == defaultValue)
		indices2[nextIndex++] = indices2[index];
}