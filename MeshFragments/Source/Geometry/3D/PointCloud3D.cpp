#include "stdafx.h"
#include "PointCloud3D.h"

#include "happly.h"

/// [Public methods]

PointCloud3D::PointCloud3D()
{
}

PointCloud3D::PointCloud3D(int numPoints, const AABB& aabb)
{
	const vec3 size = aabb.size();
	const vec3 min	= aabb.min();

	while (numPoints > 0)
	{
		const float x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (size.x * 2.0f))) + min.x;
		const float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (size.y * 2.0f))) + min.y;
		const float z = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (size.z * 2.0f))) + min.z;
		this->push_back(vec3(x, y, z));

		--numPoints;
	}
}

PointCloud3D::PointCloud3D(int numPoints, float radius)
{
	while (numPoints > 0)
	{
		const float theta	= static_cast <float> (rand()) / (static_cast <float> (RAND_MAX)) * 2.0f * glm::pi<float>();
		const float phi		= std::acos(1.0f - 2.0f * static_cast <float> (rand()) / (static_cast <float> (RAND_MAX)));
		const double x		= std::sin(phi) * std::cos(theta);
		const double y		= std::sin(phi) * std::sin(theta);
		const double z		= std::cos(phi);
		const float r		= radius * std::sqrt(static_cast <float> (rand()) / (static_cast <float> (RAND_MAX)));

		this->push_back(vec3(r * x, r * y, r * z));

		--numPoints;
	}
}

PointCloud3D::PointCloud3D(const PointCloud3D& pointCloud) :
	_points(pointCloud._points), _aabb(pointCloud._aabb)
{
}

PointCloud3D::~PointCloud3D()
{
}

vec3 PointCloud3D::getPoint(const int index) const
{
	if (index >= _points.size()) return vec3();

	return _points[index];
}

void PointCloud3D::joinPointCloud(PointCloud3D* pointCloud)
{
	if (pointCloud && pointCloud->_points.size())
	{
		for (vec4& point : pointCloud->_points)
		{
			this->push_back(point);
		}
	}
}

PointCloud3D& PointCloud3D::operator=(const PointCloud3D& pointCloud)
{
	if (this != &pointCloud)
	{
		_points = pointCloud._points;
		_aabb	= pointCloud._aabb;
	}

	return *this;
}

void PointCloud3D::push_back(const vec3& point)
{
	_points.push_back(vec4(point, 1.0f));
	_aabb.update(point);
}

void PointCloud3D::push_back(const vec4* points, unsigned numPoints)
{
	for (unsigned idx = 0; idx < numPoints; ++idx)
	{
		this->push_back(points[idx]);	
		//vec4 color = glm::unpackUnorm4x8(points[idx].w);
		_color.push_back(points[idx].w);
	}
}

std::thread* PointCloud3D::save(const std::string& filename, FractureParameters::ExportPointCloudExtension pointCloudExtension)
{
	const std::string extensionStr = FractureParameters::ExportPointCloud_STR[pointCloudExtension];
	std::thread* thread;

	if (pointCloudExtension == FractureParameters::ExportPointCloudExtension::PLY)
	{
		thread = new std::thread(&PointCloud3D::savePLY, this, filename + "." + extensionStr, std::move(_points));
	}
	else if (pointCloudExtension == FractureParameters::ExportPointCloudExtension::XYZ)
	{
		thread = new std::thread(&PointCloud3D::saveXYZ, this, filename + "." + extensionStr, std::move(_points));
	}
	else
	{
		thread = new std::thread(&PointCloud3D::saveCompressed, this, filename + "." + extensionStr, std::move(_points));
	}

	return thread;
}

void PointCloud3D::subselect(unsigned numPoints)
{
	if (numPoints >= _points.size()) return;

	std::shuffle(_points.begin(), _points.end(), std::mt19937(std::random_device()()));
	_points.resize(numPoints);
}

// Protected methods

void PointCloud3D::saveCompressed(const std::string& filename, const std::vector<glm::vec4>&& points)
{
	pcl::io::compression_Profiles_e compressionProfile = pcl::io::MED_RES_OFFLINE_COMPRESSION_WITH_COLOR;
	auto encoder = new pcl::io::OctreePointCloudCompression<pcl::PointXYZ>(compressionProfile, false);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());

	for (const vec4& point : points)
		cloud->push_back(pcl::PointXYZ(point.x, point.y, point.z));

	std::stringstream compressedData;
	encoder->encodePointCloud(cloud, compressedData);

	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (!file.is_open()) return;

	file.write(compressedData.str().c_str(), compressedData.str().length());
	file.close();
}

void PointCloud3D::savePLY(const std::string& filename, const std::vector<glm::vec4>&& points)
{
	std::vector<std::array<double, 3>> vertices(points.size());
	#pragma omp parallel for
	for (int idx = 0; idx < points.size(); ++idx)
		vertices[idx] = { points[idx].x, points[idx].z, points[idx].y };

	happly::PLYData plyOut;
	plyOut.addVertexPositions(vertices);
	plyOut.write(filename, happly::DataFormat::Binary);
}

void PointCloud3D::saveXYZ(const std::string& filename, const std::vector<glm::vec4>&& points)
{
	std::ofstream file(filename);
	if (!file.is_open()) return;

	for (const vec4& point : points)
		file << point.x << " " << point.y << " " << point.z << std::endl;

	file.close();
}

