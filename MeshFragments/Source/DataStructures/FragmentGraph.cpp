#include "stdafx.h"
#include "FragmentGraph.h"

#include "Graphics/Core/Model3D.h"

// [Public methods]

FragmentGraph::FragmentGraph(std::vector<Model3D::VertexGPUData>* vertices, std::vector<Model3D::FaceGPUData>* faces, const std::vector<float>& faceCluster): _vertices(vertices)
{
	unsigned faceIdx = 0;

	for (const Model3D::FaceGPUData& face : *faces)
	{
		GraphTriangle* triangle = new GraphTriangle;
		triangle->_vertices = face._vertices;
		triangle->_cluster = faceCluster[faceIdx];
		triangle->_faceIdx = faceIdx++;

		if (_cluster.find(triangle->_cluster) == _cluster.end())
			_cluster[triangle->_cluster] = new Cluster;

		_cluster[triangle->_cluster]->_triangles.push_back(triangle);
		_triangles.push_back(triangle);
	}

	this->initSharedVertices(*faces, vertices->size());
	this->initGraphs();
}

FragmentGraph::~FragmentGraph()
{
	for (const auto& clusterPair : _cluster)
		delete clusterPair.second;
	_cluster.clear();

	for (GraphTriangle* triangle : _triangles)
		delete triangle;
	_triangles.clear();
}

std::vector<std::vector<vec3>> FragmentGraph::getClusterBoundaries()
{
	std::vector<std::vector<vec3>> boundaries;

	for (auto& clusterPair : _cluster)
	{
		std::vector<vec3> line;
		for (int idx = 0; idx < clusterPair.second->_graph.size() - 1; ++idx)
		{
			line.push_back(_vertices->at(clusterPair.second->_graph[idx].first->_vertices[clusterPair.second->_graph[idx].second])._position);
		}

		boundaries.push_back(line);
	}

	return boundaries;
}

// [Protected methods]

void FragmentGraph::initGraphs()
{
	for (auto& pair: _cluster)
	{
		Cluster* cluster = pair.second;
		unsigned randomTriangle = 0;
		bool isBoundary = false;

		while (!isBoundary)
		{
			randomTriangle = RandomUtilities::getUniformRandomInt(0, cluster->_triangles.size() - 1);
			isBoundary = cluster->_triangles[randomTriangle]->isBoundary();
		}

		std::unordered_set<GraphTriangle*> visitedTriangles;
		GraphTriangle* triangle = cluster->_triangles[randomTriangle];
		unsigned startingVertex, vertex = 0;
		for (int i = 0; i < 3; ++i)
			if (triangle->_isBoundary[i])
				vertex = startingVertex = i;

		cluster->_graph.push_back(std::make_pair(triangle, vertex));
		visitedTriangles.insert(triangle);

		isBoundary = true;
		while (isBoundary)
		{
			isBoundary = false;
			vertex = (vertex + 1) % 3;

			for (auto& pair : triangle->_connectionVertexWise[vertex])
			{
				if (pair.first->isBoundary() && pair.first->_cluster == triangle->_cluster && visitedTriangles.find(pair.first) == visitedTriangles.end())
				{
					triangle = pair.first;
					vertex = pair.second;
					isBoundary = true;
					visitedTriangles.insert(pair.first);
				}
			}

			cluster->_graph.insert(cluster->_graph.begin(), std::make_pair(triangle, vertex));
		}

		vertex = startingVertex;
		triangle = cluster->_triangles[randomTriangle];
		isBoundary = true;

		while (isBoundary)
		{
			isBoundary = false;
			vertex = (vertex - 1 + 3) % 3;

			for (auto& pair : triangle->_connectionVertexWise[vertex])
			{
				if (pair.first->isBoundary() && pair.first->_cluster == triangle->_cluster && visitedTriangles.find(pair.first) == visitedTriangles.end())
				{
					triangle = pair.first;
					vertex = pair.second;
					isBoundary = true;
					visitedTriangles.insert(pair.first);
				}
			}

			cluster->_graph.insert(cluster->_graph.end(), std::make_pair(triangle, vertex));
		}
	}
}

void FragmentGraph::initSharedVertices(const std::vector<Model3D::FaceGPUData>& faces, size_t numVertices)
{
	std::vector<std::vector<std::pair<GraphTriangle*, uint8_t>>> vertexWiseTriangles(numVertices);

	for (size_t idx = 0; idx < faces.size(); ++idx)
		for (int i = 0; i < 3; ++i)
			vertexWiseTriangles[faces[idx]._vertices[i]].push_back(std::make_pair(_triangles[idx], static_cast<uint8_t>(i)));

	for (size_t idx = 0; idx < faces.size(); ++idx)
		for (int i = 0; i < 3; ++i)
		{
			for (const auto& pair : vertexWiseTriangles[faces[idx]._vertices[i]])
			{
				_triangles[idx]->_connectionVertexWise[i].push_back(pair);
			}

			_triangles[idx]->updateBoundary(i);
		}
}