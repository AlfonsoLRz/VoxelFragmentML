#include "stdafx.h"
#include "Bvh.h"

// Public methods

#include "Geometry/3D/Ray3D.h"
#include "Geometry/3D/TriangleMesh.h"
#include "Geometry/3D/Intersections3D.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/RenderingParameters.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Utilities/ChronoUtilities.h"

/// [Initialization of static attributes]

const GLuint Bvh::BVH_BUILDING_RADIUS = 100;

/// [Public methods]

Bvh::Bvh(Model3D* model) : _model(model)
{
	if (_model)
	{
		VolatileGPUData* volatileGPUData;
		this->aggregateSSBOData(volatileGPUData);

		// BVH generation
		const unsigned radius = BVH_BUILDING_RADIUS;
		
		ComputeShader* findNeighborShader		= ShaderList::getInstance()->getComputeShader(RendEnum::FIND_BEST_NEIGHBOR);
		ComputeShader* clusterMergingShader		= ShaderList::getInstance()->getComputeShader(RendEnum::CLUSTER_MERGING);
		ComputeShader* reallocClustersShader	= ShaderList::getInstance()->getComputeShader(RendEnum::REALLOCATE_CLUSTERS);
		ComputeShader* endLoopCompShader		= ShaderList::getInstance()->getComputeShader(RendEnum::END_LOOP_COMPUTATIONS);

		// Prefix scan
		ComputeShader* reduceShader				= ShaderList::getInstance()->getComputeShader(RendEnum::REDUCE_PREFIX_SCAN);
		ComputeShader* downSweepShader			= ShaderList::getInstance()->getComputeShader(RendEnum::DOWN_SWEEP_PREFIX_SCAN);
		ComputeShader* resetPositionShader		= ShaderList::getInstance()->getComputeShader(RendEnum::RESET_LAST_POSITION_PREFIX_SCAN);

		// Compute shader execution data: groups and iteration control
		for (const StaticGPUData& staticData : _staticGPUData)
		{
			unsigned arraySize = staticData._numTriangles, startIndex = arraySize, finishBit = 0, iteration, numExec, numThreads, startThreads;
			int numGroups, numGroups2Log;
			const int maxGroupSize = ComputeShader::getMaxGroupSize();

			// Prepare buffers for GPU
			Model3D::BVHCluster* outCluster = new Model3D::BVHCluster[arraySize];					// We declare an input buffer instead of asking for a GPU one cause we've proved it's faster

			// Compact cluster buffer support
			GLuint* currentPosBuffer = new GLuint[arraySize], * currentPosBufferOut = new GLuint[arraySize];
			std::iota(currentPosBufferOut, currentPosBufferOut + arraySize, 0);

			GLuint coutBuffer			= volatileGPUData->_tempClusterSSBO;													// Swapped during loop => not const
			GLuint cinBuffer			= ComputeShader::setReadBuffer(outCluster, arraySize);
			GLuint inCurrentPosition	= ComputeShader::setReadBuffer(currentPosBuffer, arraySize);		// Position of compact buffer where a cluster is saved
			GLuint outCurrentPosition	= ComputeShader::setReadBuffer(currentPosBufferOut, arraySize);
			const GLuint neighborIndex	= ComputeShader::setWriteBuffer(GLuint(), arraySize);				// Nearest neighbor search	
			const GLuint prefixScan		= ComputeShader::setWriteBuffer(GLuint(), arraySize);				// Final position of each valid cluster for the next loop iteration
			const GLuint validCluster	= ComputeShader::setWriteBuffer(GLuint(), arraySize);				// Clusters which takes part of next loop iteration
			const GLuint mergedCluster	= ComputeShader::setWriteBuffer(GLuint(), arraySize);				// A merged cluster is always valid, but the opposite situation is not fitting
			const GLuint numNodesCount	= ComputeShader::setReadData(arraySize);							// Number of currently added nodes, which increases as the clusters are merged
			const GLuint arraySizeCount = ComputeShader::setWriteBuffer(GLuint(), 1);

			while (arraySize > 1)
			{
				// Binary tree and whole array group sizes and iteration boundaries
				numGroups = ComputeShader::getNumGroups(arraySize);
				startThreads = std::ceil(arraySize / 2.0f);
				numExec = std::ceil(std::log2(arraySize));
				numGroups2Log = ComputeShader::getNumGroups(startThreads);

				std::vector<GLuint> threadCount{ startThreads };				// Thread sizes are repeated on reduce and sweep down phases
				threadCount.reserve(numExec);

				std::swap(coutBuffer, cinBuffer);
				std::swap(inCurrentPosition, outCurrentPosition);

				findNeighborShader->bindBuffers(std::vector<GLuint>{ cinBuffer, neighborIndex });
				findNeighborShader->use();
				findNeighborShader->setUniform("arraySize", arraySize);
				findNeighborShader->setUniform("radius", radius);
				findNeighborShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);

				clusterMergingShader->bindBuffers(std::vector<GLuint>{ cinBuffer, staticData._clusterSSBO, neighborIndex, validCluster, mergedCluster,
					prefixScan, inCurrentPosition, numNodesCount });
				clusterMergingShader->use();
				clusterMergingShader->setUniform("arraySize", arraySize);
				clusterMergingShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);

				// FIRST STEP: build a binary tree with a summatory of the array
				reduceShader->bindBuffers(std::vector<GLuint> { prefixScan });
				reduceShader->use();
				reduceShader->setUniform("arraySize", arraySize);

				iteration = 0;
				while (iteration < numExec)
				{
					numThreads = threadCount[threadCount.size() - 1];

					reduceShader->setUniform("iteration", iteration++);
					reduceShader->setUniform("numThreads", numThreads);
					reduceShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);

					threadCount.push_back(std::ceil(numThreads / 2.0f));
				}

				// SECOND STEP: set last position to zero, its faster to do it in GPU than retrieve the array in CPU, modify and write it again to GPU
				resetPositionShader->bindBuffers(std::vector<GLuint> { prefixScan });
				resetPositionShader->use();
				resetPositionShader->setUniform("arraySize", arraySize);
				resetPositionShader->execute(1, 1, 1, 1, 1, 1);

				// THIRD STEP: build tree back to first level and compute final summatory
				downSweepShader->bindBuffers(std::vector<GLuint> { prefixScan });
				downSweepShader->use();
				downSweepShader->setUniform("arraySize", arraySize);

				iteration = threadCount.size() - 2;
				while (iteration >= 0 && iteration < numExec)
				{
					downSweepShader->setUniform("iteration", iteration);
					downSweepShader->setUniform("numThreads", threadCount[iteration--]);
					downSweepShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);
				}

				reallocClustersShader->bindBuffers(std::vector<GLuint>{ cinBuffer, coutBuffer, validCluster, prefixScan, inCurrentPosition, outCurrentPosition });
				reallocClustersShader->use();
				reallocClustersShader->setUniform("arraySize", arraySize);
				reallocClustersShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);

				// Updates cluster size
				endLoopCompShader->bindBuffers(std::vector<GLuint>{ arraySizeCount, prefixScan, validCluster });
				endLoopCompShader->use();
				endLoopCompShader->setUniform("arraySize", arraySize);
				endLoopCompShader->execute(1, 1, 1, 1, 1, 1);

				arraySize = endLoopCompShader->readData(arraySizeCount, GLuint())[0];
			}

			Model3D::BVHCluster* clusterData = ComputeShader::readData(staticData._clusterSSBO, Model3D::BVHCluster());
			volatileGPUData->_cluster = std::move(std::vector<Model3D::BVHCluster>(clusterData, clusterData + staticData._numTriangles * 2 - 1));

			// Free buffers from GPU
			GLuint toDeleteBuffers[] = { coutBuffer, cinBuffer, inCurrentPosition, outCurrentPosition, neighborIndex,
										 prefixScan, validCluster, mergedCluster, numNodesCount, arraySizeCount };
			glDeleteBuffers(sizeof(toDeleteBuffers) / sizeof(GLuint), toDeleteBuffers);

			// Free dynamic memory
			delete[]	outCluster;
			delete[]	currentPosBuffer;
			delete[]	currentPosBufferOut;

			delete volatileGPUData;
		}
	}
}

Bvh::~Bvh()
{
	_staticGPUData.clear();
}

/// [Protected methods]

void Bvh::aggregateSSBOData(VolatileGPUData*& volatileGPUData)
{
	Model3D::ModelComponent* modelComponent = _model->getModelComponent(0);
	if (!modelComponent)
		return;

	unsigned numVertices = modelComponent->_geometry.size(), numTriangles = modelComponent->_topology.size();
	Model3D::MeshGPUData mesh{ numVertices, 0 };
	volatileGPUData = new VolatileGPUData;

	this->_aabb.update(modelComponent->_aabb);

	StaticGPUData staticData{};
	staticData._numTriangles		= numTriangles;
	staticData._numClusters			= staticData._numTriangles * 2 - 1;
	staticData._groupGeometrySSBO	= modelComponent->_geometrySSBO;
	staticData._groupMeshSSBO		= ComputeShader::setReadBuffer(&mesh, 1, GL_STATIC_DRAW);
	staticData._groupTopologySSBO	= modelComponent->_topologySSBO;
	const GLuint mortonCodes		= this->computeMortonCodes(&staticData);
	const GLuint sortedIndices		= this->sortFacesByMortonCode(&staticData, mortonCodes);

	this->buildClusterBuffer(volatileGPUData, &staticData, sortedIndices);
	_staticGPUData.push_back(staticData);
}

void Bvh::buildClusterBuffer(VolatileGPUData* gpuData, StaticGPUData* staticData, const GLuint sortedFaces)
{
	ComputeShader* buildClusterShader = ShaderList::getInstance()->getComputeShader(RendEnum::BUILD_CLUSTER_BUFFER);

	const unsigned arraySize = staticData->_numTriangles;
	const unsigned clusterSize = staticData->_numTriangles * 2 - 1;				// We'll only fill arraySize clusters
	const int numGroups = ComputeShader::getNumGroups(arraySize);

	Model3D::BVHCluster* clusterData	= new Model3D::BVHCluster[clusterSize], * tempClusterData = new Model3D::BVHCluster[arraySize];
	staticData->_clusterSSBO			= ComputeShader::setWriteBuffer(Model3D::BVHCluster(), clusterSize, GL_DYNAMIC_DRAW);
	gpuData->_tempClusterSSBO			= ComputeShader::setWriteBuffer(Model3D::BVHCluster(), arraySize, GL_DYNAMIC_DRAW);

	buildClusterShader->bindBuffers(std::vector<GLuint> { staticData->_groupTopologySSBO, sortedFaces, staticData->_clusterSSBO, gpuData->_tempClusterSSBO });
	buildClusterShader->use();
	buildClusterShader->setUniform("arraySize", arraySize);
	buildClusterShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	glDeleteBuffers(1, &sortedFaces);
}

GLuint Bvh::computeMortonCodes(StaticGPUData* staticData)
{
	ComputeShader* computeMortonShader = ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_MORTON_CODES);

	const unsigned arraySize = staticData->_numTriangles;
	const int numGroups = ComputeShader::getNumGroups(arraySize);
	const GLuint mortonCodeBuffer = ComputeShader::setWriteBuffer(unsigned(), arraySize);

	computeMortonShader->bindBuffers(std::vector<GLuint> { staticData->_groupTopologySSBO, mortonCodeBuffer });
	computeMortonShader->use();
	computeMortonShader->setUniform("arraySize", arraySize);
	computeMortonShader->setUniform("sceneMaxBoundary", _aabb.max());
	computeMortonShader->setUniform("sceneMinBoundary", _aabb.min());
	computeMortonShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	return mortonCodeBuffer;
}

GLuint Bvh::sortFacesByMortonCode(StaticGPUData* staticData, const GLuint mortonCodes)
{
	ComputeShader* bitMaskShader = ShaderList::getInstance()->getComputeShader(RendEnum::BIT_MASK_RADIX_SORT);
	ComputeShader* reduceShader = ShaderList::getInstance()->getComputeShader(RendEnum::REDUCE_PREFIX_SCAN);
	ComputeShader* downSweepShader = ShaderList::getInstance()->getComputeShader(RendEnum::DOWN_SWEEP_PREFIX_SCAN);
	ComputeShader* resetPositionShader = ShaderList::getInstance()->getComputeShader(RendEnum::RESET_LAST_POSITION_PREFIX_SCAN);
	ComputeShader* reallocatePositionShader = ShaderList::getInstance()->getComputeShader(RendEnum::REALLOCATE_RADIX_SORT);

	const unsigned numBits = 30;	// 10 bits per coordinate (3D)
	unsigned arraySize = staticData->_numTriangles, currentBits = 0;
	const int numGroups = ComputeShader::getNumGroups(arraySize);
	const int maxGroupSize = ComputeShader::getMaxGroupSize();
	GLuint* indices = new GLuint[arraySize];

	// Binary tree parameters
	const unsigned startThreads = std::ceil(arraySize / 2.0f);
	const int numExec = std::ceil(std::log2(arraySize));
	const int numGroups2Log = ComputeShader::getNumGroups(startThreads);
	unsigned numThreads = 0, iteration;

	// Fill indices array from zero to arraySize - 1
	for (int i = 0; i < arraySize; ++i) { indices[i] = i; }

	GLuint indicesBufferID_1, indicesBufferID_2, pBitsBufferID, nBitsBufferID, positionBufferID;
	indicesBufferID_1 = ComputeShader::setWriteBuffer(GLuint(), arraySize);
	indicesBufferID_2 = ComputeShader::setReadBuffer(indices, arraySize);					// Substitutes indicesBufferID_1 for the next iteration
	pBitsBufferID = ComputeShader::setWriteBuffer(GLuint(), arraySize);
	nBitsBufferID = ComputeShader::setWriteBuffer(GLuint(), arraySize);
	positionBufferID = ComputeShader::setWriteBuffer(GLuint(), arraySize);

	while (currentBits < numBits)
	{
		std::vector<GLuint> threadCount{ startThreads };
		threadCount.reserve(numExec);

		std::swap(indicesBufferID_1, indicesBufferID_2);							// indicesBufferID_2 is initialized with indices cause it's swapped here

		// FIRST STEP: BIT MASK, check if a morton code gives zero or one for a certain mask (iteration)
		unsigned bitMask = 1 << currentBits++;

		bitMaskShader->bindBuffers(std::vector<GLuint> { mortonCodes, indicesBufferID_1, pBitsBufferID, nBitsBufferID });
		bitMaskShader->use();
		bitMaskShader->setUniform("arraySize", arraySize);
		bitMaskShader->setUniform("bitMask", bitMask);
		bitMaskShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);

		// SECOND STEP: build a binary tree with a summatory of the array
		reduceShader->bindBuffers(std::vector<GLuint> { nBitsBufferID });
		reduceShader->use();
		reduceShader->setUniform("arraySize", arraySize);

		iteration = 0;
		while (iteration < numExec)
		{
			numThreads = threadCount[threadCount.size() - 1];

			reduceShader->setUniform("iteration", iteration++);
			reduceShader->setUniform("numThreads", numThreads);
			reduceShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);

			threadCount.push_back(std::ceil(numThreads / 2.0f));
		}

		// THIRD STEP: set last position to zero, its faster to do it in GPU than retrieve the array in CPU, modify and write it again to GPU
		resetPositionShader->bindBuffers(std::vector<GLuint> { nBitsBufferID });
		resetPositionShader->use();
		resetPositionShader->setUniform("arraySize", arraySize);
		resetPositionShader->execute(1, 1, 1, 1, 1, 1);

		// FOURTH STEP: build tree back to first level and compute position of each bit
		downSweepShader->bindBuffers(std::vector<GLuint> { nBitsBufferID });
		downSweepShader->use();
		downSweepShader->setUniform("arraySize", arraySize);

		iteration = threadCount.size() - 2;
		while (iteration >= 0 && iteration < numExec)
		{
			downSweepShader->setUniform("iteration", iteration);
			downSweepShader->setUniform("numThreads", threadCount[iteration--]);
			downSweepShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);
		}

		reallocatePositionShader->bindBuffers(std::vector<GLuint> { pBitsBufferID, nBitsBufferID, indicesBufferID_1, indicesBufferID_2 });
		reallocatePositionShader->use();
		reallocatePositionShader->setUniform("arraySize", arraySize);
		reallocatePositionShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);
	}

	glDeleteBuffers(1, &indicesBufferID_1);
	glDeleteBuffers(1, &pBitsBufferID);
	glDeleteBuffers(1, &nBitsBufferID);
	glDeleteBuffers(1, &positionBufferID);

	delete[] indices;

	return indicesBufferID_2;
}

/// VolatileGPUData

Bvh::VolatileGPUData::VolatileGPUData() : _tempClusterSSBO(-1), _mortonCodesSSBO(-1)
{
}

Bvh::VolatileGPUData::~VolatileGPUData()
{
	glDeleteBuffers(1, &_mortonCodesSSBO);
	glDeleteBuffers(1, &_tempClusterSSBO);
}

// StaticGPUData

Bvh::StaticGPUData::StaticGPUData() : _groupGeometrySSBO(-1), _groupTopologySSBO(-1), _groupMeshSSBO(-1), _clusterSSBO(-1), _numTriangles(0)
{
}

Bvh::StaticGPUData::~StaticGPUData()
{
	// Delete buffers
	GLuint toDeleteBuffers[] = { _groupMeshSSBO, _clusterSSBO };
	glDeleteBuffers(sizeof(toDeleteBuffers) / sizeof(GLuint), toDeleteBuffers);
}
