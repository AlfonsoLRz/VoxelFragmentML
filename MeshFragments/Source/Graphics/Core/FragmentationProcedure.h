#pragma once

#include "stdafx.h"
#include "Graphics/Core/FractureParameters.h"

struct FragmentationProcedure
{
	bool				_exportFragments	= true;	
	bool				_exportMetadata		= true;
	std::string			_currentDestinationFolder = "";

	FractureParameters	_fractureParameters;
	ivec2				_fragmentInterval	= ivec2(2, 12);
	ivec2				_iterationInterval	= ivec2(15, 5);
	std::string			_folder				= "Assets/Models/Modelos Vasijas OBJ (completo)-20211117T102301Z-001/Modelos OBJ (completo)";
	std::string			_destinationFolder  = "E:/Dataset/";
	std::string			_startVessel		= "";
	std::string			_saveExtension		= ".stl";
	bool				_saveScreenshots	= false;
	std::string			_searchExtension	= ".obj";

	struct FragmentMetadata
	{
		uint16_t	_id;
		std::string _vesselName;

		uint32_t	_numVertices, _numFaces;
		glm::uint	_occupiedVoxels;
		float		_percentage;
		glm::uint	_voxels;
		glm::uint16 _voxelizationSize;

		FragmentMetadata() : _id(0), _vesselName(""), _occupiedVoxels(0), _percentage(0.0f), _voxels(0), _voxelizationSize(0), _numFaces(0), _numVertices(0) {}
	};

	FragmentationProcedure()
	{
		_fractureParameters._biasSeeds = 0;
		_fractureParameters._erode = false;

		_fractureParameters._renderGrid = false;
		_fractureParameters._renderPointCloud = false;
		_fractureParameters._renderMesh = _saveScreenshots;
	}
};

typedef std::vector<FragmentationProcedure::FragmentMetadata> FragmentMetadataBuffer;