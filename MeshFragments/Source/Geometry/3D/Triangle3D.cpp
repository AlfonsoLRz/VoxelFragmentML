#include "stdafx.h"
#include "Triangle3D.h"

#include "Geometry/3D/Plane.h"
#include "Geometry/General/BasicOperations.h"

/// [Public methods]

Triangle3D::Triangle3D()
{
}

Triangle3D::Triangle3D(const vec3& va, const vec3& vb, const vec3& vc) :
	_a(va), _b(vb), _c(vc)
{
	_n = this->normal();
}

Triangle3D::Triangle3D(const Triangle3D& triangle) :
	_a(triangle._a), _b(triangle._b), _c(triangle._c), _n(triangle._n)
{
}

Triangle3D::~Triangle3D()
{
}

AABB Triangle3D::aabb()
{
	AABB aabb;
	aabb.update(_a);
	aabb.update(_b);
	aabb.update(_c);

	return aabb;
}

float Triangle3D::area() const
{
	const vec3 u = _b - _a, v = _c - _a;

	return glm::length(glm::cross(u, v)) / 2.0f;
}

Triangle3D::PointPosition Triangle3D::classify(const vec3& point)
{
	const vec3 v = point - _a;
	const float module = glm::length(v);

	if (BasicOperations::equal(module, 0.0f))
	{
		return PointPosition::COPLANAR;
	}

	const float d = glm::dot(glm::normalize(v), normal());

	if (d > glm::epsilon<float>())
	{
		return PointPosition::POSITIVE;
	}
	else if (d < glm::epsilon<float>())
	{
		return PointPosition::NEGATIVE;
	}

	return PointPosition::COPLANAR;
}

float Triangle3D::distance(const vec3& point) const
{
	Plane plane(_a, _b, _c, true);

	return plane.distance(point);
}

float Triangle3D::getAlpha(Edge3D& edge) const
{
	const vec3 normal	= this->normal();
	const vec3 u		= edge.getDest() - edge.getOrigin();
	const float num		= normal.x * u.x + normal.y * u.y + normal.z * u.z;
	const float den		= std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z) * std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);

	return std::asin(num / den);
}

float Triangle3D::getAlpha(Triangle3D& triangle) const
{
	const vec3 n_1 = this->normal(), n_2 = triangle.normal();
	const float dotN = glm::dot(n_1, n_2);

	return dotN / (glm::length(n_1) * glm::length(n_2));
}

vec3 Triangle3D::getCenterOfMass() const
{
	return (_a + _b + _c) * 1.0f / 3.0f;
}

float Triangle3D::getDihedralAngle(const Triangle3D& triangle)
{
	return glm::dot(_n, triangle._n) / (glm::length(_n) * glm::length(triangle._n));
}

vec3 Triangle3D::getRandomPoint() const
{
	vec3 u = _b - _a, v = _c - _a;
	vec2 randomFactors = vec2(RandomUtilities::getUniformRandom(.0f, 1.0f), RandomUtilities::getUniformRandom(.0f, 1.0f));

	if (randomFactors.x + randomFactors.y >= 1.0f)
	{
		randomFactors = 1.0f - randomFactors;
	}

	return _a + u * randomFactors.x + v * randomFactors.y;
}

vec3 Triangle3D::normal() const
{
	const vec3 u = _b - _a, v = _c - _a;
	const vec3 n = glm::cross(u, v);

	return glm::normalize(n);
}

Triangle3D& Triangle3D::operator=(const Triangle3D& triangle)
{
	_a = triangle._a;
	_b = triangle._b;
	_c = triangle._c;

	return *this;
}

void Triangle3D::set(const vec3& va, const vec3& vb, const vec3& vc)
{
	_a = va;
	_b = vb;
	_c = vc;
}

void Triangle3D::subdivide(std::vector<Model3D::VertexGPUData>& vertices, std::vector<Model3D::FaceGPUData>& faces, Model3D::FaceGPUData& face, float maxArea)
{
	this->subdivide(vertices, faces, face, maxArea, 2);
}

unsigned Triangle3D::longestEdgeOrigin()
{
	float l1 = glm::length(_c - _a), l2 = glm::length(_b - _a), l3 = glm::length(_c - _b);

	if (l1 > l2 && l1 > l3) return 2;
	if (l2 > l3 && l2 > l1) return 0;
	return 1;
}

void Triangle3D::subdivide(std::vector<Model3D::VertexGPUData>& vertices, std::vector<Model3D::FaceGPUData>& faces, Model3D::FaceGPUData& face, float maxArea, unsigned iteration)
{
	Triangle3D triangle(vertices[face._vertices.x]._position, vertices[face._vertices.y]._position, vertices[face._vertices.z]._position);
	if (triangle.area() <= maxArea)
	{
		faces.push_back(face);
		return;
	}

	// Retrieve longest edge
	iteration = triangle.longestEdgeOrigin();

	// Calculate midpoint
	Model3D::VertexGPUData newVertex = vertices[face._vertices.x];
	newVertex._position = (vertices[face._vertices[iteration % 3]]._position + vertices[face._vertices[(iteration + 1) % 3]]._position) / 2.0f;
	newVertex._normal = glm::normalize((vertices[face._vertices[iteration % 3]]._normal + vertices[face._vertices[(iteration + 1) % 3]]._normal) / 2.0f);
	newVertex._textCoord = (vertices[face._vertices[iteration % 3]]._textCoord + vertices[face._vertices[(iteration + 1) % 3]]._textCoord) / 2.0f;
	vertices.push_back(newVertex);

	Model3D::FaceGPUData face1 = face, face2 = face;
	face1._vertices = uvec3(vertices.size() - 1, face._vertices[(iteration + 1) % 3], face._vertices[(iteration + 2) % 3]);
	face2._vertices = uvec3(vertices.size() - 1, face._vertices[iteration % 3], face._vertices[(iteration + 2) % 3]);

	this->subdivide(vertices, faces, face1, maxArea, iteration + 1);
	this->subdivide(vertices, faces, face2, maxArea, iteration + 1);
}
