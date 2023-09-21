#include "stdafx.h"
#include "Segment3D.h"

/// [Public methods]

Segment3D::Segment3D()
{
}

Segment3D::Segment3D(const vec3& orig, const vec3& dest) :
	Edge3D(orig, dest)
{
}

Segment3D::Segment3D(const Segment3D& segment) :
	Edge3D(segment)
{
}

Segment3D::~Segment3D()
{
}

Plane Segment3D::getPlane()
{
	vec3 halfedgePoint = (_orig + _dest) / 2.0f;
	vec3 normal = glm::normalize(_direction);

	return Plane(halfedgePoint, normal);
}

Segment3D& Segment3D::operator=(const Segment3D& segment)
{
	Edge3D::operator=(segment);

	return *this;
}

/// [Protected methods]

bool Segment3D::isParametricTValid(const float t) const
{
	return (t >= 0.0f) && (t <= 1.0f);
}
