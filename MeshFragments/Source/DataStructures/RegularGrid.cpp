#include "stdafx.h"
#include "RegularGrid.h"

/// Public methods

RegularGrid::RegularGrid(const AABB& aabb, uvec3 subdivisions) :
	_aabb(aabb), _numDivs(subdivisions)
{
	_cellSize = vec3((_aabb.max().x - _aabb.min().x) / float(subdivisions.x), (_aabb.max().y - _aabb.min().y) / float(subdivisions.y), (_aabb.max().z - _aabb.min().z) / float(subdivisions.z));
	_grid = new uint16_t **[subdivisions.x];

	for (int x = 0; x < subdivisions.x; ++x)
	{
		_grid[x] = new uint16_t * [subdivisions.y];
		
		for (int y = 0; y < subdivisions.y; ++y)
		{
			_grid[x][y] = new uint16_t[subdivisions.z];
		}
	}
}

RegularGrid::~RegularGrid()
{
}

void RegularGrid::fill(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, unsigned index, int numSamples)
{
	for (const Model3D::FaceGPUData& face: faces)
	{
		int samples = numSamples;
		
		Triangle3D triangle(vertices[face._vertices.x]._position, vertices[face._vertices.y]._position, vertices[face._vertices.z]._position);
		while (samples-- >= 0) this->insertPoint(triangle.getRandomPoint(), index);
	}
}

void RegularGrid::getAABBs(std::vector<AABB>& aabb)
{
	vec3 max, min;
	
	for (int x = 0; x < _numDivs.x; ++x)
	{
		for (int y = 0; y < _numDivs.y; ++y)
		{
			for (int z = 0; z < _numDivs.z; ++z)
			{
				if (_grid[x][y][z]._index != VOXEL_EMPTY)
				{
					min = _aabb.min() + _cellSize * vec3(x, y, z);
					max = min + _cellSize;

					aabb.push_back(AABB(min, max));
				}
			}
		}
	}
}

void RegularGrid::insertPoint(const vec3& position, unsigned index)
{
	uvec3 gridIndex = getPositionIndex(position);

	_grid[gridIndex.x][gridIndex.y][gridIndex.z]._index = index;
}

/// Protected methods	

uvec3 RegularGrid::getPositionIndex(const vec3& position)
{
	unsigned x = (position.x - _aabb.min().x) / _cellSize.x, y = (position.y - _aabb.min().y) / _cellSize.y, z = (position.z - _aabb.min().z) / _cellSize.z;
	unsigned zeroUnsigned = 0;

	return uvec3(glm::clamp(x, zeroUnsigned, _numDivs.x - 1), glm::clamp(y, zeroUnsigned, _numDivs.y - 1), glm::clamp(z, zeroUnsigned, _numDivs.z - 1));
}