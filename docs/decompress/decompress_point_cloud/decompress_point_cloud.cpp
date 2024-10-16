#define _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>

#include "pcl/point_cloud.h"
#include "pcl/point_types.h"
#include "pcl/compression/octree_pointcloud_compression.h"
#include "pcl/io/ply_io.h"

int main(int argc, char** argv)
{
	std::string folder = "../samples/", extension = ".binp";

	// Find files in folder that match extension
	std::vector<std::string> files;
	for (const auto& entry : std::filesystem::directory_iterator(folder))
		if (entry.path().extension() == extension)
			files.push_back(entry.path().string());

	// Read binary file into stringstream
	for (const std::string& file : files)
	{
		std::ifstream input(file, std::ios::in | std::ios::binary);
		std::stringstream buffer;
		buffer << input.rdbuf();
		input.close();

		// Decompress point cloud
		pcl::io::OctreePointCloudCompression<pcl::PointXYZ> decompressor;
		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
		decompressor.decodePointCloud(buffer, cloud);

		// Fill until reaching the next power of 2, in case the compression removed similar points
		int size = cloud->size();
		int next = 1;
		while (next < size) next *= 2;
		
		while (cloud->size() < next)
			// Fill with a random point of those already in the cloud
			cloud->push_back(cloud->at(rand() % size));

		// Print point cloud
		std::cout << "Loaded " << cloud->size() << " points from " << file << std::endl;

		// Write to ply
		std::string ply = file.substr(0, file.size() - 5) + ".ply";
		pcl::io::savePLYFileBinary(ply, *cloud);
	}

	return 0;
}