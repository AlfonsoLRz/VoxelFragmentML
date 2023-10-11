#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer GridBuffer		{ float		grid[]; };
layout (std430, binding = 1) buffer VertexBuffer	{ vec4		vertexData[]; };
layout (std430, binding = 2) buffer FaceCounter		{ uint		numVertices; };
layout (std430, binding = 3) buffer TriangleTable	{ int		triangleTable[]; };
layout (std430, binding = 4) buffer ConfigTable		{ int		configurationTable[]; };
layout (std430, binding = 5) buffer SupportBuffer	{ vec4		vertexList[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform float isolevel;
uniform mat4 modelMatrix;
uniform int targetValue;

// The indices of the 8 neighbors that form the boundary of this cell
const ivec3 neighbors[8] =
{
	ivec3(0, 0, 0),
	ivec3(0, 0, 1),
	ivec3(-1, 0, 1),
	ivec3(-1, 0, 0),
	ivec3(0, 1, 0),
	ivec3(0, 1, 1),
	ivec3(-1, 1, 1),
	ivec3(-1, 1, 0)
};

const ivec2 edgeTable[12] =
{
	{ 0, 1 },
	{ 1, 2 },
	{ 2, 3 },
	{ 3, 0 },
	{ 4, 5 },
	{ 5, 6 },
	{ 6, 7 },
	{ 7, 4 },
	{ 0, 4 },
	{ 1, 5 },
	{ 2, 6 },
	{ 3, 7 }
};

vec3 findVertex(in ivec3 cellIndices, in float isolevel, in ivec2 edge, float value_1, float value_2)
{
	// Grab the two vertices at either end of the edge between `index_1` and `index_2`
	vec3 p1 = cellIndices + neighbors[edge.x];
	vec3 p2 = cellIndices + neighbors[edge.y];

	if (abs(isolevel - value_1) < EPSILON || abs(value_1 - value_2) < EPSILON)
	{
		return p1;
	}

	if (abs(isolevel - value_2) < EPSILON)
	{
		return p2;
	}

	const float mu = (isolevel - value_1) / (value_2 - value_1);
	vec3 p = p1 + mu * (p2 - p1);

	return p;
}

void march(in uint index, in ivec3 cellIndices)
{
	uvec3 volumeSize = gridDims;
	vec3 invVolumeSize = 1.0 / vec3(volumeSize);

	// Avoid sampling outside of the volume bounds
	if (cellIndices.x == 0 || cellIndices.y == (volumeSize.y - 1) || cellIndices.z == (volumeSize.z - 1))
		return;

	// Calculate which of the 256 configurations this cell is 
	float values[8];
	int configuration = 0;
	for (int i = 0; i < 8; ++i)
	{
		// Sample the volume texture at this neighbor's coordinates
		values[i] = float(int(grid[getPositionIndex(cellIndices + neighbors[i])]) == targetValue);

		// Compare the sampled value to the user-specified isolevel
		if (values[i] < isolevel)
			configuration |= 1 << i;
	}

	// Grab all of the (interpolated) vertices along each of the 12 edges of this cell
	for (int i = 0; i < 12; ++i)
	{
		vertexList[index * 12 + i].xyz = vec3(.0f);

		if (int(configurationTable[configuration] & (1 << i)) != 0)
		{
			ivec2 edge = edgeTable[i];
			vertexList[index * 12 + i].xyz = findVertex(cellIndices, isolevel, edge, values[edge.x], values[edge.y]);
		}
	}

	// Construct triangles based on this cell's configuration and the vertices calculated above
	const uint triangleStartMemory = configuration * 16; // 16 = the size of each "row" in the triangle table
	const uint maxTriangles = 5;

	for (int i = 0; i < maxTriangles; ++i)
	{
		if (triangleTable[triangleStartMemory + 3 * i] != -1)
		{
			uint offset = atomicAdd(numVertices, 3);

			vertexData[offset + 0].xyz = vertexList[index * 12 + triangleTable[triangleStartMemory + (3 * i + 0)]].xyz;
			vertexData[offset + 1].xyz = vertexList[index * 12 + triangleTable[triangleStartMemory + (3 * i + 1)]].xyz;
			vertexData[offset + 2].xyz = vertexList[index * 12 + triangleTable[triangleStartMemory + (3 * i + 2)]].xyz;
			//testBuffer[getPositionIndex(cellIndices)].x = int(grid[getPositionIndex(cellIndices + neighbors[0])].value);
			//testBuffer[getPositionIndex(cellIndices)].yzw = vertexData[offset + 2].position;
		}
	}
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= gridDims.x * gridDims.y * gridDims.z)
		return;

	// The 3D coordinates of this compute shader thread
	ivec3 cellIndices = ivec3(getPosition(index));

	// Perform marching cubes
	march(index, cellIndices);
}