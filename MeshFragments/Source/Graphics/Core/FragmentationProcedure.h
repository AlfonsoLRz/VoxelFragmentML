#pragma once

#include "stdafx.h"
#include "Graphics/Core/FractureParameters.h"

struct FragmentationProcedure
{
	bool				_compressResultingFiles = true;
	std::string			_currentDestinationFolder = "";

	FractureParameters	_fractureParameters;
	ivec2				_fragmentInterval = ivec2(2, 10);
	ivec2				_iterationInterval = ivec2(15, 5);
	std::string			_folder = "D:/allopezr/Datasets/Vessels_renamed/";
	std::string			_destinationFolder = "D:/allopezr/Fragments/Vessels_renamed/";
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

		_fractureParameters._exportGrid = true;
		_fractureParameters._exportMesh = true;
		_fractureParameters._exportPointCloud = true;

		_fractureParameters._exportGridExtension = FractureParameters::VOX;
		_fractureParameters._exportMeshExtension = FractureParameters::STL;
		_fractureParameters._exportPointCloudExtension = FractureParameters::ExportPointCloudExtension::PLY;
	}
};

typedef std::vector<FragmentationProcedure::FragmentMetadata> FragmentMetadataBuffer;