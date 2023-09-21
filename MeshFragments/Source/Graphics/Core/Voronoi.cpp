#include "stdafx.h"
#include "Voronoi.h"

// [Public methods]

Voronoi::Voronoi(std::vector<vec3>& points): _points(points), _segments(points.size()), _planes(points.size()), _maxDistance(points.size(), 0)
{
	this->buildDelaunay();
}

int Voronoi::getCluster(float x, float y, float z) const
{
	vec3 point(x, y, z);

	for (int pointIdx = 0; pointIdx < _points.size(); ++pointIdx)
	{
		if (glm::distance(point, _points[pointIdx]) < _maxDistance[pointIdx] + glm::epsilon<float>())
		{
			bool inside = true;
			for (const Plane& plane : _planes[pointIdx])
			{
				inside &= plane.distance(point) > -glm::epsilon<float>();
			}

			if (inside) return pointIdx;
		}
	}

	return -1;
}

// [Protected methods]

void Voronoi::buildDelaunay(/*std::vector<std::vector<Segment3D>>& segments*/)
{
	if (!_points.empty())
	{
		std::vector<Point> points;
		for (vec3& vertex : _points)
		{
			points.push_back(Point(vertex.x, vertex.y, vertex.z));
		}

		Delaunay delaunay(points.begin(), points.end());
		for (const auto edge : delaunay.finite_edges()) 
		{
			const auto v1_info = edge.first->vertex(edge.second)->point();
			const auto v2_info = edge.first->vertex(edge.third)->point();

			const int index_1 = this->findPoint(edge.first->vertex(edge.second)->point());
			const int index_2 = this->findPoint(edge.first->vertex(edge.third)->point());

			if (index_1 >= 0 and index_2 >= 0)
			{
				_segments[index_1].push_back(Segment3D(
					vec3(edge.first->vertex(edge.third)->point().x(), edge.first->vertex(edge.third)->point().y(), edge.first->vertex(edge.third)->point().z()), 
					vec3(edge.first->vertex(edge.second)->point().x(), edge.first->vertex(edge.second)->point().y(), edge.first->vertex(edge.second)->point().z()))
				);

				_segments[index_2].push_back(Segment3D(
					vec3(edge.first->vertex(edge.second)->point().x(), edge.first->vertex(edge.second)->point().y(), edge.first->vertex(edge.second)->point().z()),
					vec3(edge.first->vertex(edge.third)->point().x(), edge.first->vertex(edge.third)->point().y(), edge.first->vertex(edge.third)->point().z()))
				);
			}

			float distance = glm::distance(
				vec3(edge.first->vertex(edge.third)->point().x(), edge.first->vertex(edge.third)->point().y(), edge.first->vertex(edge.third)->point().z()),
				vec3(edge.first->vertex(edge.second)->point().x(), edge.first->vertex(edge.second)->point().y(), edge.first->vertex(edge.second)->point().z()));
			_maxDistance[index_1] = std::max(_maxDistance[index_1], distance);
			_maxDistance[index_2] = std::max(_maxDistance[index_2], distance);
		}

		for (int pointIdx = 0; pointIdx < _segments.size(); ++pointIdx)
		{
			for (int segmentIdx = 0; segmentIdx < _segments[pointIdx].size(); ++segmentIdx)
			{
				_planes[pointIdx].push_back(_segments[pointIdx][segmentIdx].getPlane());
			}
		}
	}
}

int Voronoi::findPoint(CGAL::Point_3<CGAL::Epick>& point)
{
	int index = 0;

	for (const vec3& pointSeed : _points)
	{
		if (glm::distance(pointSeed, vec3(point.x(), point.y(), point.z())) < glm::epsilon<float>())
			return index;
		++index;
	}

	return -1;
}
