#pragma once

#include "stdafx.h"
#include "Model3D.h"

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>

typedef CGAL::Simple_cartesian<double>					CartesianKernel;
typedef CGAL::Surface_mesh<CartesianKernel::Point_3>	Mesh;
typedef Mesh::Vertex_index								VertexDescriptor;
typedef Mesh::Face_index								FaceDescriptor;

namespace CGALInterface
{
	Mesh* translateMesh(Model3D::ModelComponent* modelComponent);
	void translateMesh(Mesh* mesh, Model3D::ModelComponent* modelComponent);
}

inline Mesh* CGALInterface::translateMesh(Model3D::ModelComponent* modelComponent)
{
	std::vector<VertexDescriptor> vertexDescriptor(modelComponent->_geometry.size());
	Mesh* mesh = new Mesh();

	for (size_t vertexIdx = 0; vertexIdx < modelComponent->_geometry.size(); ++vertexIdx)
	{
		vertexDescriptor[vertexIdx] = mesh->add_vertex(CartesianKernel::Point_3(modelComponent->_geometry[vertexIdx]._position.x, modelComponent->_geometry[vertexIdx]._position.y, modelComponent->_geometry[vertexIdx]._position.z));
	}

	for (size_t faceIdx = 0; faceIdx < modelComponent->_topology.size(); ++faceIdx)
	{
		mesh->add_face(
			vertexDescriptor[modelComponent->_topology[faceIdx]._vertices.x],
			vertexDescriptor[modelComponent->_topology[faceIdx]._vertices.y],
			vertexDescriptor[modelComponent->_topology[faceIdx]._vertices.z]);
	}

	return mesh;
}

inline void CGALInterface::translateMesh(Mesh* mesh, Model3D::ModelComponent* modelComponent)
{
	std::vector<unsigned> mapping(modelComponent->_geometry.size(), 0);

	modelComponent->_geometry.resize(mesh->vertices().size());
	modelComponent->_topology.resize(mesh->faces().size());

#pragma omp parallel for
	for (int idx = 0; idx < modelComponent->_geometry.size(); ++idx)
	{
		auto vertex = mesh->vertices().begin() + idx;
		mapping[vertex->idx()] = idx;

		CartesianKernel::Point_3 point = mesh->point(*vertex);
		modelComponent->_geometry[idx] = Model3D::VertexGPUData{ vec3(point.x(), point.y(), point.z()) };
	}

#pragma omp parallel for
	for (int idx = 0; idx < modelComponent->_topology.size(); ++idx)
	{
		auto face_index = mesh->faces().begin() + idx;
		CGAL::Vertex_around_face_circulator<Mesh> vcirc(mesh->halfedge(*face_index), *mesh), done(vcirc);

		std::vector<unsigned> indices;
		do indices.push_back(*vcirc++); while (vcirc != done);
		modelComponent->_topology[idx] = Model3D::FaceGPUData{ uvec3(mapping[indices[0]], mapping[indices[1]], mapping[indices[2]]) };
	}
}
