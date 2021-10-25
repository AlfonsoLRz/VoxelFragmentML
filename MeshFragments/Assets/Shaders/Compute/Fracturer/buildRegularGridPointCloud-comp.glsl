#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

layout (std430, binding = 0) buffer PointBuffer		{ vec4				points[]; };
layout (std430, binding = 1) buffer ThermalBuffer	{ float				thermalValues[]; };
layout (std430, binding = 2) buffer GridBuffer		{ uint16_t			grid[]; };
layout (std430, binding = 3) buffer AggregationBuff { ivec2				aggregation[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform vec3 aabbMin;
uniform vec3 cellSize;
uniform uint numPoints;

uvec3 getPositionIndex(vec3 position)
{
	uint x = uint(floor((position.x - aabbMin.x) / cellSize.x)), y = uint(floor((position.y - aabbMin.y) / cellSize.y)), z = uint(floor((position.z - aabbMin.z) / cellSize.z));
	uint zeroUnsigned = 0;

	return uvec3(clamp(x, zeroUnsigned, gridDims.x - 1), clamp(y, zeroUnsigned, gridDims.y - 1), clamp(z, zeroUnsigned, gridDims.z - 1));
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numPoints) return;

	vec3 point		= points[index].xyz;
	uint thermalValue = floatsBitsToUint(thermalValues[index]);
	uvec3 gridIndex = getPositionIndex(point);

	grid[getPositionIndex(gridIndex)] = VOXEL_FREE;
	atomicAdd(aggregation[gridIndex].x, thermalValue);
	atomicAdd(aggregation[gridIndex].y, 1);
}