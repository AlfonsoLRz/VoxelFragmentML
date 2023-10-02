#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer VertexBuffer	{ VertexGPUData		vertex[]; };
layout (std430, binding = 1) buffer FaceBuffer		{ FaceGPUData		face[]; };
layout (std430, binding = 2) buffer PointBuffer		{ vec4				newPoint[]; };
layout (std430, binding = 3) buffer PointCounter	{ uint				pointIndex; };
layout (std430, binding = 4) buffer NoiseBuffer		{ float				noise[]; };

uniform float		maxArea;
uniform uint		noiseBufferSize;
uniform uint		numSamples;
uniform uint		numTriangles;
uniform sampler2D	texKadSampler;

uint compressRGBA(vec4 kadColor)
{
	return packUnorm4x8(vec4(kadColor.rgb, .0f));
}

vec2 getTextCoord(vec3 position, uint a, uint b, uint c)
{
	float u, v, w;
	vec3 v0 = vertex[b].position - vertex[a].position, v1 = vertex[c].position - vertex[a].position, v2 = position - vertex[a].position;
	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;

	return u * vertex[a].textCoord + v * vertex[b].textCoord + w * vertex[c].textCoord;
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numTriangles * numSamples)
	{
		return;
	}
	
	uint sampleIdx = index % numSamples;
	uint faceIdx = uint(floor(index / numSamples));
	vec3 v1 = vertex[face[faceIdx].vertices.x].position, v2 = vertex[face[faceIdx].vertices.y].position, v3 = vertex[face[faceIdx].vertices.z].position;
	vec3 u = v2 - v1, v = v3 - v1;
	float area = length(cross(u, v)) / 2.0f;
	uint numTriangleSamples = uint(floor((area / maxArea) * numSamples));

	if (sampleIdx >= numTriangleSamples)
		return;

	vec2 randomFactors = vec2(noise[sampleIdx * 2 + 0], noise[sampleIdx * 2 + 1]);

	if (randomFactors.x + randomFactors.y >= 1.0f)
	{
		randomFactors = 1.0f - randomFactors;
	}

	vec3 point = v1 + u * randomFactors.x + v * randomFactors.y;
	vec2 textCoord = getTextCoord(point, face[faceIdx].vertices.x, face[faceIdx].vertices.y, face[faceIdx].vertices.z);

	uint newPointIndex = atomicAdd(pointIndex, 1);
	newPoint[newPointIndex] = vec4(point, float(compressRGBA(texture(texKadSampler, textCoord))));
}