#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Fracturer/voxelStructs.glsl>

layout (std430, binding = 0) buffer ClusterBuffer	{ BVHCluster				clusterData[]; };
layout (std430, binding = 1) buffer VertexBuffer	{ VertexGPUData				vertexData[]; };
layout (std430, binding = 2) buffer FaceBuffer		{ FaceGPUData				faceData[]; };
layout (std430, binding = 3) buffer MeshDataBuffer	{ MeshGPUData				meshData[]; };
layout (std430, binding = 4) buffer GridBuffer		{ CellGrid					grid[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform vec3 aabbMin;
uniform vec3 cellSize;
uniform uint numClusters;					// Start traversal from last cluster (root)
uniform uint numVoxels;


// Computes intersection between a ray and an axis-aligned bounding box. Slabs method
bool rayAABBIntersection(in RayGPUData ray, vec3 minPoint, vec3 maxPoint)
{
	vec3 tMin	= (minPoint - ray.origin) / ray.direction;
    vec3 tMax	= (maxPoint - ray.origin) / ray.direction;
    vec3 t1		= min(tMin, tMax);
    vec3 t2		= max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar	= min(min(t2.x, t2.y), t2.z);

    return tFar >= tNear;
}

// Computes intersection between a ray and a triangle
bool rayTriangleIntersection(in FaceGPUData face, in RayGPUData ray, in BVHCluster cluster)
{
	const VertexGPUData v1 = vertexData[face.vertices.x + meshData[face.modelCompID].startIndex], 
						v2 = vertexData[face.vertices.y + meshData[face.modelCompID].startIndex],
						v3 = vertexData[face.vertices.z + meshData[face.modelCompID].startIndex];

	vec3 edge1, edge2, h, s, q;
	float a, f, u, v, uv;

	edge1 = v2.position - v1.position;
	edge2 = v3.position - v1.position;

	h = cross(ray.direction, edge2);
	a = dot(edge1, h);

	if (abs(a) < EPSILON)				// Parallel ray case
	{
		return false;
	}

	f = 1.0f / a;
	s = ray.origin - v1.position;
	u = f * dot(s, h);

	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	q = cross(s, edge1);
	v = f * dot(ray.direction, q);
	uv = u + v;

	if (v < 0.0f || uv > 1.0f)
	{
		return false;
	}

	float t = f * dot(edge2, q);
	vec3 intersectionPoint = ray.origin + ray.direction * t;
	float distanceToOrigin = distance(ray.origin, intersectionPoint);

	if (t >= -EPSILON)
	{
		return true;
	}

	return false;							// There is a LINE intersection but no RAY intersection
}

bool collidedFace(in RayGPUData ray)
{
	int		currentIndex = 0;
	uint	toExplore[100];

	toExplore[currentIndex] = numClusters - 1;			// First node to explore: root

	while (currentIndex >= 0)
	{
		BVHCluster cluster = clusterData[toExplore[currentIndex]];

		if (rayAABBIntersection(ray, cluster.minPoint, cluster.maxPoint))
		{
			if (cluster.faceIndex != UINT_MAX)
			{
				if (rayTriangleIntersection(faceData[cluster.faceIndex], ray, cluster)) return true;
			}
			else
			{
				toExplore[currentIndex] = cluster.prevIndex1;
				toExplore[++currentIndex] = cluster.prevIndex2;
				++currentIndex;												// Prevent --currentIndex instead of branching
			}
		}

		--currentIndex;
	}

	return false;
}


void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numVoxels)
	{
		return;
	}

	vec3 voxelPosition = vec3(getPosition(index)) * cellSize + aabbMin + cellSize / 2.0f;
	vec3 rayDirection[6] = { vec3(.0f, 1.0f, .0f), vec3(.0f, -1.0f, .0f), vec3(1.0f, .0f, .0f), vec3(-1.0f, .0f, .0f), vec3(.0f, .0f, 1.0f), vec3(.0f, .0f, -1.0f) };
	RayGPUData ray;
	ray.origin = voxelPosition;

	for (int idx = 0; idx < 6; ++idx)
	{
		ray.direction = rayDirection[idx];
		if (!collidedFace(ray)) return;
	}

	grid[index].value = VOXEL_FREE;
}