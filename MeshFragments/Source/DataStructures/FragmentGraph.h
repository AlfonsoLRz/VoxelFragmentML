#pragma once

#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Core/Model3D.h"

class FragmentGraph
{
protected:
	struct GraphTriangle
	{
		uvec3											_vertices;
		unsigned										_faceIdx;
		uint16_t										_cluster;
		std::vector<std::pair<GraphTriangle*, uint8_t>>	_connectionVertexWise[3];
		bool											_isBoundary[3];

		bool isBoundary()
		{
			uint8_t sum = _isBoundary[0] + _isBoundary[1] + _isBoundary[2];
			return sum >= 2;
		}

		void updateBoundary(uint8_t vertex)
		{
			std::unordered_set<uint16_t> clusters;
			for (const auto& triangle : _connectionVertexWise[vertex])
				clusters.insert(triangle.first->_cluster);

			_isBoundary[vertex] = clusters.size() >= 2;
		}
	};

	struct Cluster
	{
		std::vector<FragmentGraph::GraphTriangle*>			_triangles;
		std::vector<std::pair<FragmentGraph::GraphTriangle*, uint8_t>> _graph;
	};

protected:
	std::vector<Model3D::VertexGPUData>*	_vertices;
	std::unordered_map<uint16_t, Cluster*>	_cluster;
	std::vector<GraphTriangle*>				_triangles;

protected:
	void initGraphs();
	void initSharedVertices(const std::vector<Model3D::FaceGPUData>& faces, size_t numVertices);

public:
	/**
	*	@brief Main constructor.
	*/
	FragmentGraph(std::vector<Model3D::VertexGPUData>* vertices, std::vector<Model3D::FaceGPUData>* faces, const std::vector<float>&);
	
	/**
	*	@brief Destructor.
	*/
	virtual ~FragmentGraph();

	/**
	*	@return Vector of fragment-wise boundaries.
	*/
	std::vector<std::vector<vec3>> getClusterBoundaries();
};

