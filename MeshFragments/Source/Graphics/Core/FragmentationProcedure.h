#pragma once

#include "stdafx.h"
#include "Graphics/Core/FractureParameters.h"

struct FragmentationProcedure
{
	bool				_compressResultingFiles = true;
	std::string			_currentDestinationFolder = "";

	FractureParameters	_fractureParameters;
	ivec2				_fragmentInterval = ivec2(2, 10);
	ivec2				_iterationInterval = ivec2(25, 15);
	std::string			_folder = "D:/allopezr/Datasets/Vessels_20/";
	std::string			_destinationFolder = "D:/allopezr/Fragments/Vessels_20_render/";
	size_t				_maxFragmentsModel = /*std::numeric_limits<size_t>::max()*/1000;
	std::string			_onlineFolder = "E:/Online_Testing/";
	std::string			_startVessel = "";
	std::string			_searchExtension = ".obj";

	enum FragmentType { VOXEL, POINT_CLOUD, MESH };
	struct FragmentMetadata
	{
		FragmentType	_type;
		std::string		_vesselName;
		glm::ivec3		_voxelizationSize;

		union {
			struct {
				uint32_t	_id;
				uint32_t	_numVertices;
				uint32_t	_numFaces;
				uint32_t	_occupiedVoxels;
				float		_percentage;
				uint32_t	_voxels;
			};
			uint32_t	_numPoints;
		};
	};

	FragmentationProcedure()
	{
		_fractureParameters._biasSeeds = 0;
		_fractureParameters._erode = false;
		_fractureParameters._metricVoxelization = true;

		_fractureParameters._renderGrid = false;
		_fractureParameters._renderMesh = true;
		_fractureParameters._renderPointCloud = false;

		_fractureParameters._exportGrid = false;
		_fractureParameters._exportMesh = true;
		_fractureParameters._exportPointCloud = false;

		_fractureParameters._exportGridExtension = FractureParameters::RLE;
		_fractureParameters._exportMeshExtension = FractureParameters::BINARY_MESH;
		_fractureParameters._exportPointCloudExtension = FractureParameters::ExportPointCloudExtension::COMPRESSED_POINT_CLOUD;
	}
};

typedef std::vector<FragmentationProcedure::FragmentMetadata> FragmentMetadataBuffer;