#include "stdafx.h"
#include "WingedTriangleMesh.h"

// [Public methods]

WingedTriangleMesh::WingedTriangleMesh(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, const std::vector<unsigned>& clusterIdx)
{
	_cellSize = vec3(.0f);

	for (int idx = 0; idx < faces.size(); ++idx)
	{
		Triangle3D triangle(vertices[faces[idx]._vertices.x]._position, vertices[faces[idx]._vertices.y]._position, vertices[faces[idx]._vertices.z]._position);
		AABB aabb = triangle.aabb();

		_aabb.update(vertices[faces[idx]._vertices.x]._position);
		_aabb.update(vertices[faces[idx]._vertices.y]._position);
		_aabb.update(vertices[faces[idx]._vertices.z]._position);
		_cellSize = glm::min(_cellSize, aabb.size());

		_triangle.push_back(WingedTriangle{triangle, clusterIdx[idx]});
	}

	_cellSize *= 3.0f / 4.0f;
	_numDivs = glm::ceil(_aabb.size() / _cellSize);

	_pointGrid.resize(_numDivs.x);
	for (int x = 0; x < _numDivs.x; ++x)
	{
		_pointGrid[x].resize(_numDivs.y);
		for (int y = 0; y < _numDivs.y; ++y)
		{
			_pointGrid[y].resize(_numDivs.z);
		}
	}

	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		uvec3 position;
		for (int i = 0; i < 3; ++i)
		{
			position = this->getPositionIndex(_triangle[idx]._triangle.getPoint(i));
			_pointGrid[position.x][position.y][position.z].push_back(std::make_pair(idx, i));
		}
	}

	this->checkVertexConnectivity();
}

WingedTriangleMesh::~WingedTriangleMesh()
{

}

// [Protected methods]

void WingedTriangleMesh::checkVertexConnectivity()
{
	for (int x = 0; x < _numDivs.x; ++x)
	{
		for (int y = 0; y < _numDivs.y; ++y)
		{
			for (int z = 0; z < _numDivs.z; ++z)
			{
				std::vector<std::pair<unsigned, unsigned>>* vertices = &_pointGrid[x][y][z];
				for (int i = 0; i < vertices->size(); ++i)
				{
					for (int j = i; j < vertices->size(); ++j)
					{
						if (glm::distance(_triangle[vertices->at(i).first]._triangle.getPoint(vertices->at(i).second),
							_triangle[vertices->at(j).first]._triangle.getPoint(vertices->at(j).second)) < glm::epsilon<float>())
						{
							_triangle[vertices->at(i).first]._connectionVertex.insert(vertices->at(j).first);
							_triangle[vertices->at(j).first]._connectionVertex.insert(vertices->at(i).first);
						}
					}
				}
			}
		}
	}
}

uvec3 WingedTriangleMesh::getPositionIndex(const vec3& position)
{
	unsigned x = (position.x - _aabb.min().x) / _cellSize.x, y = (position.y - _aabb.min().y) / _cellSize.y, z = (position.z - _aabb.min().z) / _cellSize.z;
	unsigned zeroUnsigned = 0;

	return uvec3(glm::clamp(x, zeroUnsigned, _numDivs.x - 1), glm::clamp(y, zeroUnsigned, _numDivs.y - 1), glm::clamp(z, zeroUnsigned, _numDivs.z - 1));
}

unsigned WingedTriangleMesh::getPositionIndex(int x, int y, int z) const
{
	return x * _numDivs.y * _numDivs.z + y * _numDivs.z + z;
}
