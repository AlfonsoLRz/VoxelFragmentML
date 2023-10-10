#include "stdafx.h"
#include "WingedTriangleMesh.h"

// [Public methods]

WingedTriangleMesh::WingedTriangleMesh(
	const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, std::vector<std::unordered_map<unsigned, float>> faceClusterOccupancy)
{
	_cellSize = vec3(.0f);

	for (int idx = 0; idx < faceClusterOccupancy.size(); ++idx)
	{
		Triangle3D triangle(vertices[faces[idx]._vertices.x]._position, vertices[faces[idx]._vertices.y]._position, vertices[faces[idx]._vertices.z]._position);
		AABB aabb = triangle.aabb();

		_aabb.update(vertices[faces[idx]._vertices.x]._position);
		_aabb.update(vertices[faces[idx]._vertices.y]._position);
		_aabb.update(vertices[faces[idx]._vertices.z]._position);
		_cellSize = glm::max(_cellSize, aabb.size());
		_triangle.push_back(new WingedTriangle{triangle, faceClusterOccupancy[idx], static_cast<unsigned>(idx) });
	}

	_cellSize /= 2.0f;
	_numDivs = glm::ceil(_aabb.size() / _cellSize);

	_pointGrid.resize(_numDivs.x);
	for (int x = 0; x < _numDivs.x; ++x)
	{
		_pointGrid[x].resize(_numDivs.y);
		for (int y = 0; y < _numDivs.y; ++y)
		{
			_pointGrid[x][y].resize(_numDivs.z);
		}
	}

	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		uvec3 position;
		for (int i = 0; i < 3; ++i)
		{
			position = this->getPositionIndex(_triangle[idx]->_triangle.getPoint(i));
			_pointGrid[position.x][position.y][position.z].push_back(std::make_pair(_triangle[idx], i));
		}
	}

	this->checkVertexConnectivity();

	_pointGrid.clear();

	//this->eraseFlatTriangles();
	//
	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		float maxProb = .0f;
		for (const auto& pair : _triangle[idx]->_cluster)
			if (pair.second > maxProb)
			{
				_triangle[idx]->_finalCluster = pair.first;
				maxProb = pair.second;
			}
	}

	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		unsigned boundaries = 0;
		for (int i = 0; i < 3; ++i)
		{
			std::unordered_set<unsigned> cluster;
			for (auto& pair : _triangle[idx]->_connectionVertexWise[i])
				cluster.insert(pair.first->_finalCluster);

			boundaries += unsigned(cluster.size() > 1);
		}

		_triangle[idx]->_boundary = boundaries >= 2;
	}
}

WingedTriangleMesh::~WingedTriangleMesh()
{
	for (WingedTriangle* triangle : _triangle)
		delete triangle;
	_triangle.clear();
}

void WingedTriangleMesh::computeAlgebraicConvexity()
{
	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		WingedTriangle* triangle = _triangle[idx];
		triangle->_test = triangle->_finalCluster;

		if (!triangle->_boundary)
			continue;

		for (int i = 0; i < 3; ++i)
		{
			unsigned newCluster = triangle->_finalCluster;
			auto neighbourIt = triangle->_connectionVertexWise[i].begin();
			while (neighbourIt != triangle->_connectionVertexWise[i].end())
			{
				if (neighbourIt->first->_finalCluster != newCluster)
				{
					newCluster = neighbourIt->first->_finalCluster;
					break;
				}
				++neighbourIt;
			}

			if (newCluster == triangle->_finalCluster)
				continue;

			float maxDot = FLT_MAX;
			WingedTriangle* neighbourTriangle = nullptr;
			vec3 b = triangle->_triangle.getPoint(i);
			vec3 a = triangle->_triangle.getPoint((i - 1 + 3) % 3);
			vec3 v1 = glm::normalize(b - a);

			for (const auto& neighbourPair : triangle->_connectionVertexWise[i])
			{
				if (neighbourPair.first->_finalCluster == triangle->_finalCluster)
				{
					vec3 c = neighbourPair.first->_triangle.getPoint((neighbourPair.second + 1) % 3);
					vec3 v2 = glm::normalize(c - a);
					float angle = glm::dot(v1, v2);

					if (angle < maxDot)
					{
						maxDot = angle;
						neighbourTriangle = neighbourPair.first;
					}
				}
			}

			if (neighbourTriangle && maxDot < 0.0f)
			{
				//neighbourTriangle->_finalCluster = newCluster;
				//neighbourTriangle->_boundary = false;
				//neighbourTriangle->_test = true;
				triangle->_test = triangle->_finalCluster + 1;
			}
		}
	}
}

void WingedTriangleMesh::computeHardCluster(float smoothness)
{
	//	E(x¯) = ∑f∈Fe1(f, xf) + λ∑{ f,g }∈Ne2(xf, xg)
	//	e1(f, xf) = −log(max(P(f | xf), ϵ1))
	//	e2(xf, xg) = { −log(wmax(1− | θ(f,g)|/π,ϵ2))0 xf≠xg xf = xg }
	std::vector<unsigned> newCluster(_triangle.size());
	float epsilon1 = glm::epsilon<float>(), epsilon2 = glm::epsilon<float>();

	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		WingedTriangle* triangle = _triangle[idx];
		float minEnergy = FLT_MAX;
		unsigned minCluster = triangle->_finalCluster;

		newCluster[idx] = triangle->_finalCluster;

		std::unordered_set<unsigned> clusterIdx;
		this->getClusters(idx, clusterIdx);

		if (clusterIdx.size() <= 1)
			continue;

		for (auto cluster : clusterIdx)
		{
			float energy = .0f;

			auto clusterIt = triangle->_cluster.find(cluster);
			if (clusterIt != triangle->_cluster.end())
			{
				energy += -log(glm::max(clusterIt->second, epsilon1));
			}

			for (WingedTriangle* neighbour : triangle->_connectionVertex)
			{
				if (neighbour->_finalCluster != cluster)
				{
					float dihedrical = glm::acos(triangle->_triangle.getDihedralAngle(neighbour->_triangle)) / glm::pi<float>(), w = (dihedrical < .0f) ? .08f : 1.0f;
					energy += smoothness * -log(w * glm::max(1.0f - glm::abs(dihedrical), epsilon2));
				}
			}

			if (energy < minEnergy)
			{
				minEnergy = energy;
				minCluster = cluster;
			}
		}

		triangle->_finalCluster = minCluster;
	}

	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		_triangle[idx]->_finalCluster = newCluster[idx];
	}
}

void WingedTriangleMesh::computeSoftCluster(float threshold)
{
	std::vector<unsigned> newCluster(_triangle.size());

#pragma omp parallel for
	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		WingedTriangle* triangle = _triangle[idx];
		float maxOccupancy = .0f;
		unsigned finalCluster = triangle->_finalCluster;

		newCluster[idx] = triangle->_finalCluster;

		std::unordered_set<unsigned> clusterIdx;
		this->getClusters(idx, clusterIdx);

		if (clusterIdx.size() <= 1)
			continue;

		for (auto cluster : clusterIdx)
		{
			float occupancy = .0f;

			auto clusterIt = triangle->_cluster.find(cluster);
			//if (clusterIt != triangle->_cluster.end())
			//{
			//	++occupancy;
			//}

			for (WingedTriangle* neighbour : triangle->_connectionVertex)
			{
				clusterIt = neighbour->_cluster.find(cluster);
				if (clusterIt != triangle->_cluster.end())
				{
					++occupancy;
				}
			}

			if (occupancy > maxOccupancy)
			{
				maxOccupancy = occupancy;
				finalCluster = cluster;
			}
		}

		if (triangle->_finalCluster != finalCluster)
			std::cout << "Hola" << std::endl;

		triangle->_finalCluster = finalCluster;
	}

#pragma omp parallel for
	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		_triangle[idx]->_finalCluster = newCluster[idx];
	}
}

void WingedTriangleMesh::getFaceCluster(unsigned numFaces, std::vector<float>& clusterIdx)
{
#pragma omp parallel for
	for (int idx = 0; idx < _triangle.size(); ++idx)
		//if (_triangle[idx]->_test)
		//	clusterIdx[_triangle[idx]->_face] = 700;
		//else
			clusterIdx[_triangle[idx]->_face] = _triangle[idx]->_test;
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
				std::vector<std::pair<WingedTriangle*, unsigned>>* vertices = &_pointGrid[x][y][z];
				for (int i = 0; i < vertices->size(); ++i)
				{
					for (int j = i; j < vertices->size(); ++j)
					{
						if (glm::distance(vertices->at(i).first->_triangle.getPoint(vertices->at(i).second),
							vertices->at(j).first->_triangle.getPoint(vertices->at(j).second)) < glm::epsilon<float>())
						{
							vertices->at(i).first->_connectionVertex.insert(vertices->at(j).first);
							vertices->at(i).first->_connectionVertexWise[vertices->at(i).second][vertices->at(j).first] = vertices->at(j).second;

							vertices->at(j).first->_connectionVertex.insert(vertices->at(i).first);
							vertices->at(j).first->_connectionVertexWise[vertices->at(j).second][vertices->at(i).first] = vertices->at(i).second;
						}
					}
				}
			}
		}
	}
}

void WingedTriangleMesh::eraseFlatTriangles()
{
	std::vector<bool> removable(_triangle.size());

#pragma omp parallel for
	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		std::unordered_set<unsigned> clusters;
		this->getClusters(idx, clusters);

		removable[idx] = clusters.size() < 2;
	}

	unsigned erasedTriangles = 0;
	for (int idx = 0; idx < _triangle.size(); ++idx)
	{
		if (removable[idx])
		{
			delete _triangle[idx - erasedTriangles];
			_triangle.erase(_triangle.begin() + idx - erasedTriangles);
			++erasedTriangles;
		}
	}
}

void WingedTriangleMesh::getClusters(unsigned idx, std::unordered_set<unsigned>& clusterIdx)
{
	for (const auto& cluster : _triangle[idx]->_cluster)
		clusterIdx.insert(cluster.first);

	for (const auto neighbor : _triangle[idx]->_connectionVertex)
		for (const auto& cluster : neighbor->_cluster)
			clusterIdx.insert(cluster.first);
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

bool WingedTriangleMesh::processTriangle(unsigned idx)
{
	std::unordered_set<unsigned> clusters;
	this->getClusters(idx, clusters);

	return clusters.size() > 1;
}
