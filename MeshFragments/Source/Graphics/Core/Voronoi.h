#pragma once

#include "Geometry/3D/Segment3D.h"

class Voronoi
{
protected:
	std::vector<float>						_maxDistance;
	std::vector<std::vector<Plane>>			_planes;
	std::vector<vec3>						_points;
	std::vector<std::vector<Segment3D>>		_segments;

protected:
	/**
	*	@brief Builds Delaunay triangulation from a set of points.
	*/
	void buildDelaunay(/*std::vector<std::vector<Segment3D>>& segments*/);

	/**
	*	@return Index of point.
	*/
	int findPoint(CGAL::Point_3<CGAL::Epick>& point);

public:
	/**
	*	@brief Voronoi from a set of points.
	*/
	Voronoi(std::vector<vec3>& points);

	/**
	*	@return Cluster where the specified point belongs to.
	*/
	int getCluster(float x, float y, float z) const;
};

