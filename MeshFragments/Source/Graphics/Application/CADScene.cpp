#include "stdafx.h"
#include "CADScene.h"

#include "DataStructures/FragmentGraph.h"
#include "DataStructures/WingedTriangleMesh.h"
#include "Geometry/3D/PointCloud3D.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/AABBSet.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/DrawLines.h"
#include "Graphics/Core/DrawPointCloud.h"
#include "Graphics/Core/FractureParameters.h"
#include "Graphics/Core/FragmentationProcedure.h"
#include "Graphics/Core/Light.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/Voronoi.h"
#include "progressbar.hpp"
#include "Utilities/ChronoUtilities.h"
#include "Utilities/FileManagement.h"

/// Initialization of static attributes
//const std::string CADScene::TARGET_PATH = "D:/PyCharm/BlenderRenderer/assets/GU_033.obj";
const std::string CADScene::TARGET_PATH = "E:/Research/teapot.obj";

// [Public methods]

CADScene::CADScene() :
	_aabbRenderer(nullptr), _fragmentBoundaries(nullptr), _generateDataset(false), _mesh(nullptr), _meshGrid(nullptr), _pointCloud(nullptr), _pointCloudRenderer(nullptr)
{
	_aabbRenderer = new AABBSet();
	_aabbRenderer->load();
	_aabbRenderer->setMaterial(MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_CAD_BLUE));

	srand(_fractParameters._seed);
	RandomUtilities::initSeed(_fractParameters._seed);
}

CADScene::~CADScene()
{
	delete _aabbRenderer;
	delete _fragmentBoundaries;
	delete _mesh;
	delete _meshGrid;
	delete _pointCloud;
	delete _pointCloudRenderer;
}

void CADScene::exportGrid()
{
	const std::string filename = "Output/" + _mesh->getShortName() + "/";
	if (!std::filesystem::exists(filename)) std::filesystem::create_directory(filename);

	_meshGrid->exportGrid(false, filename + "grid");
}

void CADScene::exportFragments(const FractureParameters& fractureParameters, const std::string& extension)
{
	// Is folder created
	const std::string folder = "Output/" + _mesh->getShortName() + "/";
	if (!std::filesystem::exists(folder)) std::filesystem::create_directory(folder);

	for (int idx = 0; idx < _fractureMeshes.size(); ++idx)
		dynamic_cast<CADModel*>(_fractureMeshes[idx])->save(folder + "mesh_" + std::to_string(idx) + "." + extension, extension);

	for (int idx = 0; idx < _fractureMeshes.size(); ++idx)
	{
		for (int targetCount : fractureParameters._targetTriangles)
		{
			dynamic_cast<CADModel*>(_fractureMeshes[idx])->simplify(targetCount);
			dynamic_cast<CADModel*>(_fractureMeshes[idx])->save(folder + "mesh_" + std::to_string(idx) + "_" + std::to_string(targetCount) + "." + extension, extension);
		}	
	}
}

void CADScene::exportPointClouds(const FractureParameters& fractureParameters)
{
	const std::string folder = "Output/" + _mesh->getShortName() + "/";
	if (!std::filesystem::exists(folder)) std::filesystem::create_directory(folder);

	for (int idx = 0; idx < _fragmentMetadata.size(); ++idx)
	{
		for (int targetCount : fractureParameters._targetPoints)
		{
			PointCloud3D* pc = dynamic_cast<CADModel*>(_fractureMeshes[idx])->sampleCPU(std::round(targetCount * _fragmentMetadata[idx]._percentage), fractureParameters._pointCloudSeedingRandom);
			pc->save(folder + "pc_" + std::to_string(idx) + "_" + std::to_string(targetCount) + ".ply");
		}
	}
}

std::string CADScene::fractureGrid(std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters)
{
	this->eraseFragmentContent();
	if (!_generateDataset && _meshGrid)
		this->allocateMeshGrid(_fractParameters);
	this->rebuildGrid(fractureParameters);
	std::string result = this->fractureModel(fractureParameters);
	this->prepareScene(fractureParameters, fragmentMetadata);

	return result;
}

std::string CADScene::fractureGrid(const std::string& path, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters)
{
	this->eraseFragmentContent();

	if (!path.empty())
	{
		this->loadModel(path);
		if (fractureParameters._exportProcessedMesh)
		{
			for (int targetPoints : fractureParameters._targetPoints)
			{
				PointCloud3D* pointCloud = _mesh->sample(targetPoints, fractureParameters._pointCloudSeedingRandom);
				pointCloud->save("Output/" + _mesh->getShortName() + "_" + std::to_string(targetPoints) + "_points.ply");
				delete pointCloud;
			}
		}
	}

	if (!_generateDataset)
	{
		if (!_meshGrid)
			_meshGrid = new RegularGrid(ivec3(_fractParameters._clampVoxelMetricUnit));

		this->allocateMeshGrid(_fractParameters);
	}

	this->rebuildGrid(fractureParameters);
	std::string result = this->fractureModel(fractureParameters);
	this->prepareScene(fractureParameters, fragmentMetadata);

	return result;
}

void CADScene::generateDataset(FragmentationProcedure& fractureProcedure, const std::string& folder, const std::string& extension, const std::string& destinationFolder)
{
	std::vector<std::string> fileList;
	FileManagement::searchFiles(folder, extension, fileList);
	std::vector<FragmentationProcedure::FragmentMetadata> fragmentMetadata;

	fractureProcedure._currentDestinationFolder = destinationFolder;
	if (!std::filesystem::exists(fractureProcedure._currentDestinationFolder)) std::filesystem::create_directory(fractureProcedure._currentDestinationFolder);

	if (!fractureProcedure._startVessel.empty())
	{
		do
		{
			fileList.erase(fileList.begin());
		} 
		while (!fileList.empty() && fileList[0].find(fractureProcedure._startVessel) == std::string::npos);
	}

	this->_generateDataset = true;
	this->allocateMemoryDataset(fractureProcedure);

	for (const std::string& path : fileList)
	{
		std::vector<FragmentationProcedure::FragmentMetadata> modelMetadata;
		this->loadModel(path);

		const std::string modelName = _mesh->getShortName();
		const std::string meshFolder = fractureProcedure._currentDestinationFolder + modelName + "/";
		const std::string meshFile = meshFolder + modelName + "_";
		if (!std::filesystem::exists(meshFolder)) std::filesystem::create_directory(meshFolder);

		// Calculate size of voxelization according to model size
		const AABB aabb = _mesh->getAABB();
		fractureProcedure._fractureParameters._voxelizationSize = glm::ceil(aabb.size() * vec3(fractureProcedure._fractureParameters._voxelPerMetricUnit));
		if (fractureProcedure._fractureParameters._voxelizationSize.x > fractureProcedure._fractureParameters._clampVoxelMetricUnit or
			fractureProcedure._fractureParameters._voxelizationSize.y > fractureProcedure._fractureParameters._clampVoxelMetricUnit or
			fractureProcedure._fractureParameters._voxelizationSize.z > fractureProcedure._fractureParameters._clampVoxelMetricUnit)
		{
			fractureProcedure._fractureParameters._voxelizationSize =
				glm::floor(vec3(fractureProcedure._fractureParameters._clampVoxelMetricUnit) *
					aabb.size() / glm::max(aabb.size().x, glm::max(aabb.size().y, aabb.size().z)));
			std::cout << modelName << " - " << "Voxelization size clamped to " << fractureProcedure._fractureParameters._clampVoxelMetricUnit << std::endl;
		}
		while (fractureProcedure._fractureParameters._voxelizationSize.x % 4 != 0) ++fractureProcedure._fractureParameters._voxelizationSize.x;
		while (fractureProcedure._fractureParameters._voxelizationSize.z % 4 != 0) ++fractureProcedure._fractureParameters._voxelizationSize.z;

		std::cout << modelName << " - " << fractureProcedure._fractureParameters._voxelizationSize.x << "x" << fractureProcedure._fractureParameters._voxelizationSize.y << "x" << fractureProcedure._fractureParameters._voxelizationSize.z << std::endl;

		// Initialize grid content
		_meshGrid->setAABB(_mesh->getAABB(), fractureProcedure._fractureParameters._voxelizationSize);
		_meshGrid->fill(_mesh->getModelComponent(0));
		_meshGrid->resetMarchingCubes();
		_mesh->getModelComponent(0)->releaseMemory();

		for (int numFragments = fractureProcedure._fragmentInterval.x; numFragments <= fractureProcedure._fragmentInterval.y; ++numFragments)
		{
			const std::string fragmentFile = meshFile + std::to_string(numFragments) + "f_";

			fractureProcedure._fractureParameters._numExtraSeeds = 2;
			fractureProcedure._fractureParameters._numSeeds = numFragments;
			const int numIterations = glm::mix(
				fractureProcedure._iterationInterval.x, fractureProcedure._iterationInterval.y,
				static_cast<float>(numFragments - fractureProcedure._fragmentInterval.x) / (fractureProcedure._fragmentInterval.y - fractureProcedure._fragmentInterval.x));

			std::cout << modelName << " - " << numFragments << " fragments ";
			progressbar bar(numIterations);

			for (int iteration = 0; iteration < numIterations; ++iteration)
			{
				bar.update();

				unsigned idx = 0;
				const std::string itFile = fragmentFile + std::to_string(iteration) + "it";
				std::vector<FragmentationProcedure::FragmentMetadata> localMetadata;

				this->fractureGrid(fragmentMetadata, fractureProcedure._fractureParameters);

				for (Model3D* fracture : _fractureMeshes)
				{
					CADModel* cadModel = dynamic_cast<CADModel*>(fracture);
					const std::string filename = itFile + "_" + std::to_string(idx);
					std::string simplificationFilename;

					if (!fractureProcedure._fractureParameters._targetTriangles.empty())
					{
						for (int targetCount : fractureProcedure._fractureParameters._targetTriangles)
						{
							simplificationFilename = filename + "_" + std::to_string(targetCount) + "." + fractureProcedure._saveExtension;
							cadModel->simplify(targetCount);
							cadModel->save(simplificationFilename, fractureProcedure._saveExtension);

							fragmentMetadata[idx]._vesselName = simplificationFilename;
							fragmentMetadata[idx]._numVertices = fracture->getNumVertices();
							fragmentMetadata[idx]._numFaces = fracture->getNumFaces();
							localMetadata.push_back(fragmentMetadata[idx]);
						}
					}
					else
					{
						cadModel->save(filename + "." + fractureProcedure._saveExtension, fractureProcedure._saveExtension);

						fragmentMetadata[idx]._vesselName = filename + fractureProcedure._saveExtension;
						fragmentMetadata[idx]._numVertices = fracture->getNumVertices();
						fragmentMetadata[idx]._numFaces = fracture->getNumFaces();
						localMetadata.push_back(fragmentMetadata[idx]);
					}

					++idx;
				}

				modelMetadata.insert(modelMetadata.end(), localMetadata.begin(), localMetadata.end());
				fragmentMetadata.clear();
			}

			std::cout << std::endl;
		}

		if (fractureProcedure._exportMetadata)
		{
			this->exportMetadata(meshFile + "metadata.txt", modelMetadata);
		}

		if (fractureProcedure._compressFiles)
		{
			std::thread compressFolder(&CADScene::launchZipingProcess, this, meshFolder);
			compressFolder.detach();
		}

		//if (!fractureProcedure._onlineFolder.empty())
		//{
		//	if (!std::filesystem::exists(fractureProcedure._onlineFolder)) std::filesystem::create_directory(fractureProcedure._onlineFolder);

		//	std::thread copyFolder(&CADScene::launchCopyingProcess, this, meshFolder, fractureProcedure._onlineFolder + modelName);
		//	copyFolder.detach();
		//}
	}
}

void CADScene::hit(const Model3D::RayGPUData& ray)
{
	if (_meshGrid)
	{
		uvec3 hit = _meshGrid->getClosestEntryVoxel(ray);
		if (hit.x == std::numeric_limits<glm::uint>::max()) return;

		_impactSeeds.push_back(uvec4(hit, VOXEL_FREE + 1));
		this->fractureGrid(_fragmentMetadata, _fractParameters);
		_impactSeeds.clear();
	}
}

void CADScene::render(const mat4& mModel, RenderingParameters* rendParams)
{
	SSAOScene::render(mModel, rendParams);
}

// [Protected methods]

void CADScene::allocateMemoryDataset(FragmentationProcedure& fractureProcedure)
{
	// Prepare regular grid
	_meshGrid = new RegularGrid(ivec3(fractureProcedure._fractureParameters._clampVoxelMetricUnit));

	// Prepare GPU memory for fracturing
	fracturer::Fracturer* fracturer = nullptr;
	if (fractureProcedure._fractureParameters._fractureAlgorithm == FractureParameters::NAIVE)
		fracturer = fracturer::NaiveFracturer::getInstance();
	else
		fracturer = fracturer::FloodFracturer::getInstance();

	fractureProcedure._fractureParameters._voxelizationSize = ivec3(fractureProcedure._fractureParameters._clampVoxelMetricUnit);
	fracturer->prepareSSBOs(&fractureProcedure._fractureParameters);
}

void CADScene::allocateMeshGrid(FractureParameters& fractParameters)
{
	const AABB aabb = _mesh->getAABB();
	fractParameters._voxelizationSize = 
		glm::floor(vec3(glm::max(fractParameters._voxelizationSize.x, glm::max(fractParameters._voxelizationSize.y, fractParameters._voxelizationSize.z))) 
			* aabb.size() / glm::max(aabb.size().x, glm::max(aabb.size().y, aabb.size().z)));

	while (fractParameters._voxelizationSize.x % 4 != 0) ++fractParameters._voxelizationSize.x;
	while (fractParameters._voxelizationSize.z % 4 != 0) ++fractParameters._voxelizationSize.z;

	_meshGrid->setAABB(aabb, fractParameters._voxelizationSize);
	_meshGrid->fill(_mesh->getModelComponent(0));
	_meshGrid->resetMarchingCubes();

	if (fractParameters._exportStartingGrid)
	{
		_meshGrid->exportGrid(true, "Output/" + _mesh->getShortName() + "_grid.vox");
	}
}

void CADScene::eraseFragmentContent()
{
	delete _pointCloud;
	_pointCloud = nullptr;

	delete _pointCloudRenderer;
	_pointCloudRenderer = nullptr;

	for (Model3D* fractureMesh : _fractureMeshes) delete fractureMesh;
	_fractureMeshes.clear();

	for (Material* material : _fragmentMaterials) delete material;
	for (Texture* texture : _fragmentTextures) delete texture;
	_fragmentMaterials.clear();
	_fragmentTextures.clear();
}

void CADScene::exportMetadata(const std::string& filename, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentSize)
{
	std::ofstream outputStream(filename.c_str());

	if (outputStream.fail()) return;

	outputStream << "Filename\tFragment id\tVoxelization size\tVoxels\tOccupied voxels\tPercentage\tVertices\tFaces" << std::endl;

	for (int idx = 0; idx < fragmentSize.size(); ++idx)
	{
		outputStream <<
			fragmentSize[idx]._vesselName << "\t" <<
			fragmentSize[idx]._id << "\t" <<
			fragmentSize[idx]._voxelizationSize.x << "x" << fragmentSize[idx]._voxelizationSize.y << "x" << fragmentSize[idx]._voxelizationSize.z << "\t" <<
			fragmentSize[idx]._voxels << "\t" <<
			fragmentSize[idx]._occupiedVoxels << "\t" <<
			fragmentSize[idx]._percentage << "\t" <<
			fragmentSize[idx]._numVertices << "\t" <<
			fragmentSize[idx]._numFaces << std::endl;
	}

	outputStream.close();
}

std::string CADScene::fractureModel(FractureParameters& fractParameters)
{
	fracturer::DistanceFunction dfunc = static_cast<fracturer::DistanceFunction>(fractParameters._distanceFunction);

	std::vector<uvec4> seeds;
	std::vector<float> faceClusterIdx, vertexClusterIdx;
	std::vector<unsigned> boundaryFaces;
	std::vector<std::unordered_map<unsigned, float>> faceClusterOccupancy;

	if (_impactSeeds.empty())
	{
		if (fractParameters._numImpacts == 0)
		{
			seeds = fracturer::Seeder::uniform(*_meshGrid, fractParameters._numSeeds, fractParameters._seedingRandom);
		}
		else
		{
			seeds = fracturer::Seeder::uniform(*_meshGrid, fractParameters._numSeeds, fractParameters._seedingRandom);
			seeds = fracturer::Seeder::nearSeeds(*_meshGrid, seeds, fractParameters._numImpacts, fractParameters._biasSeeds, fractParameters._biasFocus);
		}
	}
	else
	{
		seeds = _impactSeeds;
		seeds = fracturer::Seeder::nearSeeds(*_meshGrid, seeds, 1, fractParameters._biasSeeds, fractParameters._biasFocus);
	}

	if (fractParameters._numExtraSeeds > 0)
	{
		fracturer::DistanceFunction mergeDFunc = static_cast<fracturer::DistanceFunction>(fractParameters._mergeSeedsDistanceFunction);
		auto extraSeeds = fracturer::Seeder::uniform(*_meshGrid, fractParameters._numExtraSeeds, fractParameters._seedingRandom);
		extraSeeds.insert(extraSeeds.begin(), seeds.begin(), seeds.end());

		fracturer::Seeder::mergeSeeds(seeds, extraSeeds, mergeDFunc);
		seeds = extraSeeds;
	}

	if (fractParameters._fractureAlgorithm != FractureParameters::VORONOI)
	{
		fracturer::Fracturer* fracturer = nullptr;
		if (fractParameters._fractureAlgorithm == FractureParameters::NAIVE)
			fracturer = fracturer::NaiveFracturer::getInstance();
		else
			fracturer = fracturer::FloodFracturer::getInstance();

		if (!fracturer->setDistanceFunction(dfunc)) return "Invalid distance function";
		fracturer->build(*_meshGrid, seeds, &fractParameters);
	}
	else
	{
		std::vector<vec3> seeds3;
		for (const vec4& seed : seeds)
			seeds3.push_back(seed + vec4(.5f));

		Voronoi voronoi(seeds3);
		_meshGrid->fill(voronoi);
		_meshGrid->updateSSBO();
	}

	if (fractParameters._erode)
	{
		_meshGrid->erode(static_cast<FractureParameters::ErosionType>(
			fractParameters._erosionConvolution), fractParameters._erosionSize, fractParameters._erosionIterations,
			fractParameters._erosionProbability, fractParameters._erosionThreshold);
	}
	else
	{
		_meshGrid->detectBoundaries(1);
	}

	return "";
}

void CADScene::launchZipingProcess(const std::string& folder)
{
	//std::filesystem::copy(folder, destinationFolder, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	const std::string currentPath = std::filesystem::current_path().string();
	const std::string systemUnit = currentPath.substr(0, currentPath.find_first_of("/\\"));
	const std::string cd = "cd " + currentPath;
	const std::string command = cd + " && " + systemUnit + " && " + "Bash\\zip.bat " + folder + " " + folder.substr(0, folder.find_first_of("/\\")) + " >nul";

	std::system(command.c_str());
}

void CADScene::loadDefaultCamera(Camera* camera)
{
	if (_mesh)
	{
		AABB aabb = _mesh->getAABB();

		camera->setLookAt(aabb.center());
		camera->setPosition(aabb.center() + aabb.extent() * 1.5f);
	}
}

void CADScene::loadDefaultLights()
{
	Light* pointLight_01 = new Light();
	pointLight_01->setLightType(Light::POINT_LIGHT);
	pointLight_01->setPosition(vec3(5.5f, 4.0f, -1.0f));
	pointLight_01->setId(vec3(0.5f));
	pointLight_01->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(pointLight_01));

	Light* sunLight = new Light();
	Camera* camera = sunLight->getCamera();
	ShadowMap* shadowMap = sunLight->getShadowMap();
	camera->setBottomLeftCorner(vec2(-7.0f, -7.0f));
	shadowMap->modifySize(4096, 4096);
	sunLight->setLightType(Light::DIRECTIONAL_LIGHT);
	sunLight->setPosition(vec3(.0f, 5.0f, -8.0f));
	sunLight->setDirection(vec3(-0.1, -0.8f, 1.0f));
	sunLight->setId(vec3(0.5f));
	sunLight->setIs(vec3(0.0f));
	sunLight->castShadows(true);
	sunLight->setShadowIntensity(0.0f, 1.0f);
	sunLight->setBlurFilterSize(5);

	_lights.push_back(std::unique_ptr<Light>(sunLight));

	Light* fillLight = new Light();
	fillLight->setLightType(Light::DIRECTIONAL_LIGHT);
	fillLight->setDirection(vec3(-1.0f, 1.0f, 0.0f));
	fillLight->setId(vec3(0.1f));
	fillLight->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(fillLight));

	Light* rimLight = new Light();
	rimLight->setLightType(Light::RIM_LIGHT);
	rimLight->setIa(vec3(0.015f, 0.015f, 0.05f));

	_lights.push_back(std::unique_ptr<Light>(rimLight));
}

void CADScene::loadCameras()
{
	ivec2 canvasSize = _window->getSize();
	Camera* camera = new Camera(canvasSize[0], canvasSize[1]);

	_cameraManager->insertCamera(camera);
}

void CADScene::loadLights()
{
	this->loadDefaultLights();
	Scene::loadLights();
}

void CADScene::loadModels()
{
	_fragmentMetadata.clear();

	this->fractureGrid(TARGET_PATH, _fragmentMetadata, _fractParameters);
	this->loadDefaultCamera(_cameraManager->getActiveCamera());
}

void CADScene::loadModel(const std::string& path)
{
	delete _mesh;
	_mesh = new CADModel(path, true, true, false);
	_mesh->load();
	_mesh->setMaterial(MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_CAD_WHITE));

	_mesh->sampleCPU(50000000, 0)->save("Output/teapot/complete.ply");
}

void CADScene::prepareScene(FractureParameters& fractParameters, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FragmentationProcedure* datasetProcedure)
{
	Texture* whiteTexture = TextureList::getInstance()->getTexture(CGAppEnum::TEXTURE_WHITE);
	_fractureMeshes = _meshGrid->toTriangleMesh(fractParameters, fragmentMetadata);

	if (fractParameters._renderMesh)
	{
		for (int idx = 0; idx < _fractureMeshes.size(); ++idx)
		{
			Material* material = new Material;
			Texture* kad = new Texture(vec4(ColorUtilities::HSVtoRGB(ColorUtilities::getHueValue(idx), 1.0f, 1.0f), 1.0f));
			material->setTexture(Texture::KAD_TEXTURE, kad);
			material->setTexture(Texture::KS_TEXTURE, whiteTexture);
			material->setShininess(500.0f);
			_fractureMeshes[idx]->setMaterial(material);

			_fragmentMaterials.push_back(material);
			_fragmentTextures.push_back(kad);
		}
	}

	_meshGrid->undoMask();

	if (fractParameters._renderGrid)
	{
		std::vector<AABB> aabbs;

		_meshGrid->getAABBs(aabbs);
		_aabbRenderer->load(aabbs);
		_aabbRenderer->setColorIndex(_meshGrid->data(), _meshGrid->getNumSubdivisions().x * _meshGrid->getNumSubdivisions().y * _meshGrid->getNumSubdivisions().z);
	}

	if (fractParameters._renderPointCloud)
	{
		_pointCloud = _mesh->sample(100, fractParameters._pointCloudSeedingRandom);
		_pointCloudRenderer = new DrawPointCloud(_pointCloud);

		std::vector<float> vertexClusterIdx;
		_meshGrid->queryCluster(_pointCloud->getPoints(), vertexClusterIdx);
		_pointCloudRenderer->getModelComponent(0)->setClusterIdx(vertexClusterIdx);
	}

	if (datasetProcedure && datasetProcedure->_exportFragments)
	{
		if (datasetProcedure->_saveScreenshots)
		{

		}
	}
}

void CADScene::rebuildGrid(FractureParameters& fractureParameters)
{
	_meshGrid->resetFilling();
}

// [Rendering]

void CADScene::drawAsTriangles(Camera* camera, const mat4& mModel, RenderingParameters* rendParams)
{
	RendEnum::RendShaderTypes shaderType = RendEnum::TRIANGLE_MESH_SHADER;
	if (rendParams->_triangleMeshRendering == RenderingParameters::VOXELIZATION)
		shaderType = RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_SHADER;
	else if (rendParams->_triangleMeshRendering == RenderingParameters::ORIGINAL_MESH_T or rendParams->_triangleMeshRendering == RenderingParameters::FRAGMENTED_MESH_T)
		shaderType = RendEnum::TRIANGLE_MESH_SHADER;
	else
		shaderType = RendEnum::CLUSTER_SHADER;

	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(shaderType);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());
	const mat4 bias = glm::translate(mat4(1.0f), vec3(0.5f)) * glm::scale(mat4(1.0f), vec3(0.5f));						// Proj: [-1, 1] => with bias: [0, 1]

	{
		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_MATRIX] = camera->getViewMatrix();
		matrix[RendEnum::VIEW_PROJ_MATRIX] = camera->getViewProjMatrix();

		glDepthFunc(GL_LEQUAL);
	}

	{
		for (unsigned int i = 0; i < _lights.size(); ++i)																// Multipass rendering
		{
			if (i == 0)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}

			if (_lights[i]->shouldCastShadows())
			{
				matrix[RendEnum::BIAS_VIEW_PROJ_MATRIX] = bias * _lights[i]->getCamera()->getViewProjMatrix();
			}

			shader->use();
			shader->setUniform("materialScattering", rendParams->_materialScattering);
			_lights[i]->applyLight(shader, matrix[RendEnum::VIEW_MATRIX]);
			_lights[i]->applyShadowMapTexture(shader);
			shader->applyActiveSubroutines();

			this->drawSceneAsTriangles(shader, shaderType, &matrix, rendParams);
		}
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);					// Back to initial state
	glDepthFunc(GL_LESS);
}

void CADScene::drawSceneAsPoints(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (rendParams->_pointCloudRendering == RenderingParameters::ORIGINAL_MESH_PC)
	{
		if (_mesh)
			_mesh->drawAsPoints(shader, shaderType, *matrix);
	}
	else if (rendParams->_pointCloudRendering == RenderingParameters::SAMPLED_POINT_CLOUD && _pointCloudRenderer)
	{
		if (rendParams->_pointCloudRendering == RenderingParameters::SAMPLED_POINT_CLOUD)
			shader->setSubroutineUniform(GL_VERTEX_SHADER, "colorUniform", "clusterColor");
		else if (rendParams->_pointCloudRendering == RenderingParameters::ORIGINAL_MESH_PC)
			shader->setSubroutineUniform(GL_VERTEX_SHADER, "colorUniform", "uniformColor");
		//else
			//shader->setSubroutineUniform(GL_VERTEX_SHADER, "colorUniform", "textureColor");
		shader->applyActiveSubroutines();

		_pointCloudRenderer->drawAsPoints(shader, shaderType, *matrix);
	}
	else
	{
		for (Model3D* fractureMesh : _fractureMeshes)
			fractureMesh->drawAsPoints(shader, shaderType, *matrix);
	}
}

void CADScene::drawSceneAsLines(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (rendParams->_wireframeRendering == RenderingParameters::ORIGINAL_MESH_W)
	{
		if (_mesh)
			_mesh->drawAsLines(shader, shaderType, *matrix);
	}
	else if (rendParams->_wireframeRendering == RenderingParameters::FRAGMENTED_MESH_W)
	{
		for (Model3D* fractureMesh : _fractureMeshes)
			fractureMesh->drawAsLines(shader, shaderType, *matrix);
	}
}

void CADScene::drawSceneAsTriangles(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_SHADER)
	{
		if (rendParams->_triangleMeshRendering == RenderingParameters::ORIGINAL_MESH_T)
		{
			if (_mesh)
				_mesh->drawAsTriangles(shader, shaderType, *matrix);
		}
		else if (rendParams->_triangleMeshRendering == RenderingParameters::FRAGMENTED_MESH_T)
		{
			for (Model3D* fractureMesh : _fractureMeshes)
				fractureMesh->drawAsTriangles(shader, shaderType, *matrix);
		}
	}
	else if (shaderType == RendEnum::CLUSTER_SHADER)
	{
		if (_mesh)
			_mesh->drawAsTriangles(shader, shaderType, *matrix);
	}
	else
	{
		if (rendParams->_planeClipping)
			shader->setUniform("planeCoefficients", rendParams->_planeCoefficients);

		_aabbRenderer->drawAsTriangles(shader, shaderType, *matrix);
	}
}

void CADScene::drawSceneAsTriangles4Normal(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_NORMAL_SHADER)
	{
		if (rendParams->_triangleMeshRendering == RenderingParameters::ORIGINAL_MESH_T or rendParams->_triangleMeshRendering == RenderingParameters::CLUSTERED_MESH)
		{
			if (_mesh)
				_mesh->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
		else if (rendParams->_triangleMeshRendering == RenderingParameters::FRAGMENTED_MESH_T)
		{
			for (Model3D* fractureMesh : _fractureMeshes)
				fractureMesh->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
	}
	else
	{
		if (rendParams->_planeClipping)
			shader->setUniform("planeCoefficients", rendParams->_planeCoefficients);

		_aabbRenderer->drawAsTriangles4Shadows(shader, shaderType, *matrix);
	}
}

void CADScene::drawSceneAsTriangles4Position(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_POSITION_SHADER or shaderType == RendEnum::SHADOWS_SHADER)
	{
		if (rendParams->_triangleMeshRendering == RenderingParameters::ORIGINAL_MESH_T or rendParams->_triangleMeshRendering == RenderingParameters::CLUSTERED_MESH)
		{
			if (_mesh)
				_mesh->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
		else if (rendParams->_triangleMeshRendering == RenderingParameters::FRAGMENTED_MESH_T)
		{
			for (Model3D* fractureMesh : _fractureMeshes)
				fractureMesh->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
	}
	else
	{
		if (rendParams->_planeClipping)
			shader->setUniform("planeCoefficients", rendParams->_planeCoefficients);

		_aabbRenderer->drawAsTriangles4Shadows(shader, shaderType, *matrix);
	}
}