#pragma once

#include "stdafx.h"
#include "Graphics/Core/FractureParameters.h"

struct FragmentationProcedure
{
	bool				_compressResultingFiles = false;
	std::string			_currentDestinationFolder = "";

	FractureParameters	_fractureParameters;
	ivec2				_fragmentInterval = ivec2(2, 12);
	ivec2				_iterationInterval = ivec2(15, 5);
	std::string			_folder = "D:/allopezr/Datasets/ModelNet40/ModelNet40/";
	std::string			_destinationFolder = "D:/allopezr/Fragments/";
	std::string			_onlineFolder = "E:/Online_Testing/";
	std::string			_startVessel = "";
	std::string			_searchExtension = ".off";

	enum FragmentType { VOXEL, POINT_CLOUD, MESH };
	struct FragmentMetadata
	{
		FragmentType	_type;
		std::string		_vesselName;
		glm::ivec3		_voxelizationSize;

		union {
			struct {
				uint8_t		_id;
				uint32_t	_numVertices;
				uint32_t	_numFaces;
				glm::uint	_occupiedVoxels;
				float		_percentage;
				glm::uint	_voxels;
			};
			uint32_t	_numPoints;
		};
	};

	FragmentationProcedure()
	{
		_fractureParameters._biasSeeds = 0;
		_fractureParameters._erode = false;
		_fractureParameters._metricVoxelization = true;
	}
};

typedef std::vector<FragmentationProcedure::FragmentMetadata> FragmentMetadataBuffer;