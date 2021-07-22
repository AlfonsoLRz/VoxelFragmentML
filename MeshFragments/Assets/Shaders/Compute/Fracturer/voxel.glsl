#define VOXEL_EMPTY uint16_t(0)		// 0 when a voxel is empy
#define VOXEL_FREE  uint16_t(1)		// 1 when a voxel is not empty and not occupied by anyone

uniform uvec3	gridDims;

uvec3 getPosition(uint index)
{
	float x = index / float(gridDims.y * gridDims.z);
	float w = index % (gridDims.y * gridDims.z);
	float y = w / gridDims.z;
	float z = uint(w) % gridDims.z;

	return uvec3(x, y, z);
}

uint getPositionIndex(uvec3 position)
{
	return position.x * gridDims.y * gridDims.z + position.y * gridDims.z + position.z;
}