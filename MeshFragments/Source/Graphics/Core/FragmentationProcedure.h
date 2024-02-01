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

	struct FragmentMetadata
	{
		uint8_t		_id;
		std::string _vesselName;

		uint32_t	_numVertices, _numFaces, _numPoints;
		glm::uint	_occupiedVoxels;
		float		_percentage;
		glm::uint	_voxels;
		glm::ivec3	_voxelizationSize;

		FragmentMetadata() : _id(0), _vesselName(""), _occupiedVoxels(0), _percentage(0.0f), _voxels(0), _voxelizationSize(0), _numFaces(0), _numVertices(0), _numPoints(0) {}
	};

	FragmentationProcedure()
	{
		_fractureParameters._biasSeeds = 0;
		_fractureParameters._erode = false;
		_fractureParameters._metricVoxelization = true;
	}
};

typedef std::vector<FragmentationProcedure::FragmentMetadata> FragmentMetadataBuffer;