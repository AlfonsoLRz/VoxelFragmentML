#include "stdafx.h"
#include "ShaderList.h"

// [Static members initialization]

std::unordered_map<uint8_t, std::string> ShaderList::COMP_SHADER_SOURCE {
		{RendEnum::ASSIGN_FACE_CLUSTER, "Assets/Shaders/Compute/Fracturer/assignFaceCluster"},
		{RendEnum::ASSIGN_VERTEX_CLUSTER, "Assets/Shaders/Compute/Fracturer/assignVertexCluster"},
		{RendEnum::BIT_MASK_RADIX_SORT, "Assets/Shaders/Compute/RadixSort/bitMask-radixSort"},
		{RendEnum::BUILD_CLUSTER_BUFFER, "Assets/Shaders/Compute/BVHGeneration/buildClusterBuffer"},
		{RendEnum::BUILD_MARCHING_CUBES_FACES, "Assets/Shaders/Compute/Fracturer/buildMarchingCubesFaces"},
		{RendEnum::BUILD_REGULAR_GRID, "Assets/Shaders/Compute/Fracturer/buildRegularGrid"},
		{RendEnum::BVH_COLLISION, "Assets/Shaders/Compute/Collision/computeBVHCollision"},
		{RendEnum::CLUSTER_MERGING, "Assets/Shaders/Compute/BVHGeneration/clusterMerging"},
		{RendEnum::COMPUTE_FACE_AABB, "Assets/Shaders/Compute/Model/computeFaceAABB"},
		{RendEnum::COMPUTE_GROUP_AABB, "Assets/Shaders/Compute/Group/computeGroupAABB"},
		{RendEnum::COMPUTE_MORTON_CODES, "Assets/Shaders/Compute/BVHGeneration/computeMortonCodes"},
		{RendEnum::COMPUTE_MORTON_CODES_FRACTURER, "Assets/Shaders/Compute/Fracturer/computeMortonCodes"},
		{RendEnum::COMPUTE_TANGENTS_1, "Assets/Shaders/Compute/Model/computeTangents_1"},
		{RendEnum::COMPUTE_TANGENTS_2, "Assets/Shaders/Compute/Model/computeTangents_2"},
		{RendEnum::COPY_GRID, "Assets/Shaders/Compute/Fracturer/copyGrid"},
		{RendEnum::COUNT_QUADRANT_OCCUPANCY, "Assets/Shaders/Compute/Fracturer/countQuadrantOccupancy"},
		{RendEnum::COUNT_VOXEL_TRIANGLE, "Assets/Shaders/Compute/Fracturer/countVoxelTriangle"},
		{RendEnum::DETECT_BOUNDARIES, "Assets/Shaders/Compute/Fracturer/detectBoundaries"},
		{RendEnum::DISJOINT_SET, "Assets/Shaders/Compute/Fracturer/disjointSet"},
		{RendEnum::DISJOINT_SET_STACK, "Assets/Shaders/Compute/Fracturer/disjointSetStack"},
		{RendEnum::DOWN_SWEEP_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/downSweep-prefixScan"},
		{RendEnum::END_LOOP_COMPUTATIONS, "Assets/Shaders/Compute/BVHGeneration/endLoopComputations"},
		{RendEnum::ERODE_GRID, "Assets/Shaders/Compute/Fracturer/erodeGrid"},
		{RendEnum::FILL_REGULAR_GRID, "Assets/Shaders/Compute/Collision/fillRegularGrid"},
		{RendEnum::FILL_REGULAR_GRID_VOXEL, "Assets/Shaders/Compute/Fracturer/fillRegularGrid"},
		{RendEnum::FIND_BEST_NEIGHBOR, "Assets/Shaders/Compute/BVHGeneration/findBestNeighbor"},
		{RendEnum::FINISH_FILL, "Assets/Shaders/Compute/Fracturer/finishFill"},
		{RendEnum::FINISH_LAPLACIAN_SMOOTHING, "Assets/Shaders/Compute/Fracturer/finishLaplacianSmoothing"},
		{RendEnum::FLOOD_FRACTURER, "Assets/Shaders/Compute/Fracturer/floodFracturer"},
		{RendEnum::FUSE_VERTICES_01, "Assets/Shaders/Compute/Fracturer/findSameVertices_01"},
		{RendEnum::FUSE_VERTICES_02, "Assets/Shaders/Compute/Fracturer/findSameVertices_02"},
		{RendEnum::LAPLACIAN_SMOOTHING, "Assets/Shaders/Compute/Fracturer/laplacianSmoothing"},
		{RendEnum::MARCHING_CUBES, "Assets/Shaders/Compute/Fracturer/marchingCubes"},
		{RendEnum::MARK_BOUNDARY_TRIANGLES, "Assets/Shaders/Compute/Fracturer/markBoundaryTriangles"},
		{RendEnum::MESH_BVH_COLLISION, "Assets/Shaders/Compute/Collision/computeMeshBVHCollision"},
		{RendEnum::MODEL_APPLY_MODEL_MATRIX, "Assets/Shaders/Compute/Model/modelApplyModelMatrix"},
		{RendEnum::MODEL_MESH_GENERATION, "Assets/Shaders/Compute/Model/modelMeshGeneration"},
		{RendEnum::NAIVE_FRACTURER, "Assets/Shaders/Compute/Fracturer/naiveFracturer"},
		{RendEnum::PLANAR_SURFACE_GENERATION, "Assets/Shaders/Compute/PlanarSurface/planarSurfaceGeometryTopology"},
		{RendEnum::PLANAR_SURFACE_TOPOLOGY, "Assets/Shaders/Compute/PlanarSurface/planarSurfaceFaces"},
		{RendEnum::REALLOCATE_CLUSTERS, "Assets/Shaders/Compute/BVHGeneration/reallocateClusters"},
		{RendEnum::REALLOCATE_RADIX_SORT, "Assets/Shaders/Compute/RadixSort/reallocateIndices-radixSort"},
		{RendEnum::REDUCE_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/reduce-prefixScan"},
		{RendEnum::REMOVE_ISOLATED_REGIONS, "Assets/Shaders/Compute/Fracturer/removeIsolatedRegions"},
		{RendEnum::REMOVE_ISOLATED_REGIONS_GRID, "Assets/Shaders/Compute/Fracturer/removeIsolatedRegionsGrid"},
		{RendEnum::RESET_BUFFER_INDEX, "Assets/Shaders/Compute/Generic/resetBufferIndex"},
		{RendEnum::RESET_BUFFER, "Assets/Shaders/Compute/Fracturer/resetBuffer"},
		{RendEnum::RESET_LAPLACIAN_SMOOTHING, "Assets/Shaders/Compute/Fracturer/resetLaplacianBuffer"},
		{RendEnum::RESET_LAST_POSITION_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/resetLastPosition-prefixScan"},
		{RendEnum::SAMPLER_SHADER, "Assets/Shaders/Compute/Model/sampler"},
		{RendEnum::SAMPLER_ALT_SHADER, "Assets/Shaders/Compute/Model/samplerAlt"},
		{RendEnum::SELECT_VOXEL_TRIANGLE, "Assets/Shaders/Compute/Fracturer/selectVoxelTriangle"},
		{RendEnum::UNDO_MASK_SHADER, "Assets/Shaders/Compute/Fracturer/undoMask"}
};

std::unordered_map<uint8_t, std::string> ShaderList::REND_SHADER_SOURCE {
		{RendEnum::BLUR_SHADER, "Assets/Shaders/Filters/blur"},
		{RendEnum::BLUR_SSAO_SHADER, "Assets/Shaders/2D/blurSSAOShader"},
		{RendEnum::BVH_SHADER, "Assets/Shaders/Lines/bvh"},
		{RendEnum::CLUSTER_SHADER, "Assets/Shaders/Triangles/clusterShader"},
		{RendEnum::DEBUG_QUAD_SHADER, "Assets/Shaders/Triangles/debugQuad"},
		{RendEnum::MULTI_INSTANCE_SHADOWS_SHADER, "Assets/Shaders/Triangles/multiInstanceShadowsShader"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMeshPosition"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMeshNormal"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMesh"},
		{RendEnum::NORMAL_MAP_SHADER, "Assets/Shaders/Filters/normalMapShader"},
		{RendEnum::POINT_CLOUD_SHADER, "Assets/Shaders/Points/pointCloud"},
		{RendEnum::POINT_CLOUD_HEIGHT_SHADER, "Assets/Shaders/Points/pointCloudHeight"},
		{RendEnum::TRIANGLE_MESH_SHADER, "Assets/Shaders/Triangles/triangleMesh"},
		{RendEnum::TRIANGLE_MESH_NORMAL_SHADER, "Assets/Shaders/Triangles/triangleMeshNormal"},
		{RendEnum::TRIANGLE_MESH_POSITION_SHADER, "Assets/Shaders/Triangles/triangleMeshPosition"},
		{RendEnum::WIREFRAME_SHADER, "Assets/Shaders/Lines/wireframe"},
		{RendEnum::SHADOWS_SHADER, "Assets/Shaders/Triangles/shadowsShader"},
		{RendEnum::SSAO_SHADER, "Assets/Shaders/2D/ssaoShader"},
		{RendEnum::VERTEX_NORMAL_SHADER, "Assets/Shaders/Lines/vertexNormal"}
};

std::vector<std::unique_ptr<ComputeShader>> ShaderList::_computeShader (RendEnum::numComputeShaderTypes());
std::vector<std::unique_ptr<RenderingShader>> ShaderList::_renderingShader (RendEnum::numRenderingShaderTypes());

/// [Protected methods]

ShaderList::ShaderList()
{
}

/// [Public methods]

ComputeShader* ShaderList::getComputeShader(const RendEnum::CompShaderTypes shader)
{
	const int shaderID = shader;

	if (!_computeShader[shader].get())
	{
		ComputeShader* shader = new ComputeShader();
		shader->createShaderProgram(COMP_SHADER_SOURCE.at(shaderID).c_str());

		_computeShader[shaderID].reset(shader);
	}

	return _computeShader[shaderID].get();
}

RenderingShader* ShaderList::getRenderingShader(const RendEnum::RendShaderTypes shader)
{
	const int shaderID = shader;

	if (!_renderingShader[shader].get())
	{
		RenderingShader* shader = new RenderingShader();
		shader->createShaderProgram(REND_SHADER_SOURCE.at(shaderID).c_str());

		_renderingShader[shaderID].reset(shader);
	}

	return _renderingShader[shader].get();
}
