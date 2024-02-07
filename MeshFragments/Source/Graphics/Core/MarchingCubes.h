#pragma once

#include "DataStructures/RegularGrid.h"
#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Core/CADModel.h"

class MarchingCubes
{
protected:
	static const int _triangleTable[256 * 16];
	static const int _edgeTable[256];

protected:
	unsigned        _gridSubdivisions;
	unsigned*		_indices;
	unsigned        _maxTriangles;
	uvec3           _numDivs;
	unsigned        _numGroups;
	unsigned        _numThreads;
	uvec3           _steps;

	// Shaders
	ComputeShader* _computeMortonShader;
	ComputeShader* _bitMaskShader;
	ComputeShader* _reduceShader;
	ComputeShader* _downSweepShader;
	ComputeShader* _resetPositionShader;
	ComputeShader* _reallocatePositionShader;

	ComputeShader* _buildMarchingCubesShader;
	ComputeShader* _fuseSimilarVerticesShader_01, * _fuseSimilarVerticesShader_02;
	ComputeShader* _marchingCubesShader;
	ComputeShader* _resetBufferShader;

	ComputeShader* _finishLaplacianShader, * _laplacianShader, * _resetLaplacianShader;
	ComputeShader* _markBoundaryTrianglesShader;

	// SSBOs
	GLuint          _edgeTableSSBO;
	GLuint          _gridSSBO;
	GLuint          _mortonCodeSSBO;
	GLuint          _nonUpdatedVerticesSSBO;
	GLuint          _numVerticesSSBO;
	GLuint          _supportVerticesSSBO;
	GLuint          _triangleTableSSBO;
	GLuint          _verticesSSBO;

	GLuint          _indicesBufferID_1;
	GLuint          _indicesBufferID_2;
	GLuint          _pBitsBufferID;
	GLuint          _nBitsBufferID;

	GLuint          _vertexSSBO;
	GLuint          _faceSSBO;

protected:
	/**
	*   @brief Builds faces with fused vertices.
	*/
	void buildMarchingCubesFaces(unsigned numVertices);

	/**
	*   @brief Calculates Morton codes from vertex buffer.
	*/
	void calculateMortonCodes(unsigned numVertices);

	/**
	*   @brief Fuses similar vertices.
	*/
	unsigned fuseSimilarVertices(unsigned numVertices, const mat4& modelMatrix);

	/**
	*   @brief Marks triangles as boundary or not.
	*/
	void markBoundaryTriangles(unsigned numFaces);

	/**
	*   @brief Resets counter for number of vertices in the GPU.
	*/
	void resetCounter(GLuint ssbo);

	/**
	*   @brief Smooths the surface using the Laplacian operator.
	*/
	void smoothSurface(unsigned numVertices, unsigned numFaces, unsigned numIterations, float weight = 1.0f, bool boundary = false);

	/**
	*   @brief Sorts previously computed Morton codes.
	*/
	void sortMortonCodes(unsigned numVertices);

public:
	/**
	*   @brief Constructor.
	*/
	MarchingCubes(RegularGrid& regularGrid, unsigned subdivisions, const uvec3& numDivs, unsigned maxTriangles = 5);

	/**
	*   @brief Destructor.
	*/
	virtual ~MarchingCubes();

	/**
	*   @brief Modifies the content related to the grid.
	*/
	void setGrid(RegularGrid& regularGrid);

	/**
	*   @brief Triangulate a scalar field represented by `scalarFunction`. `isovalue` should be used for isovalue computation.
	*/
	CADModel* triangulateFieldGPU(GLuint gridSSBO, uint16_t targetValue, FractureParameters& fractureParams, const mat4& modelMatrix);

	// Getters

	/**
	*	@brief Returns the SSBO containing the grid.
	*/
	GLuint getGridSSBO() const { return _gridSSBO; }
};

