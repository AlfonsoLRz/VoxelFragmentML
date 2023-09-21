#include "stdafx.h"
#include "CADScene.h"

#include <filesystem>
#include <regex>
#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/Light.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/Voronoi.h"
#include "Utilities/ChronoUtilities.h"

/// Initialization of static attributes
const std::string CADScene::SCENE_ROOT_FOLDER = "Assets/Scene/Basement/";
const std::string CADScene::SCENE_SETTINGS_FOLDER = "Assets/Scene/Settings/Basement/";

const std::string CADScene::SCENE_CAMERA_FILE = "Camera.txt";
const std::string CADScene::SCENE_LIGHTS_FILE = "Lights.txt";

const std::string CADScene::MESH_1_PATH = "Assets/Models/Jar01/Jar01_v3";

// [Public methods]

CADScene::CADScene() : _aabbRenderer(nullptr), _mesh(nullptr), _meshGrid(nullptr)
{
}

CADScene::~CADScene()
{
	delete _aabbRenderer;
	delete _mesh;
	delete _meshGrid;
}

void CADScene::exportGrid()
{
	_meshGrid->exportGrid();
}

std::string CADScene::fractureGrid()
{
	return this->fractureModel();
}

void CADScene::loadModel(const std::string& path)
{
	// Delete resources
	{
		delete _aabbRenderer;
		_aabbRenderer = nullptr;

		delete _meshGrid;
		_meshGrid = nullptr;

		for (Group3D* group : _sceneGroup) delete group;
		_sceneGroup.clear();

		for (Model3D* fractureMesh : _fractureMeshes) delete fractureMesh;
		_fractureMeshes.clear();
	}

	{
		_mesh = new CADModel(path, path.substr(0, path.find_last_of("/") + 1), true);
		_mesh->load();
		_mesh->getModelComponent(0)->_enabled = true;

		Group3D* group = new Group3D();
		group->addComponent(_mesh);
		group->registerScene();
		group->generateBVH(_sceneGPUData, true);
		_sceneGroup.push_back(group);

		// Build octree and retrieve AABBs
		_aabbRenderer = new AABBSet();
		_aabbRenderer->load();
		_aabbRenderer->setMaterial(MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_CAD_BLUE));

		this->rebuildGrid();
		this->fractureModel();
	}

	this->loadDefaultCamera(_cameraManager->getActiveCamera());
}

void CADScene::rebuildGrid()
{
	std::vector<AABB> aabbs;
	std::vector<float> clusterIdx (_mesh->getModelComponent(0)->_geometry.size());

	delete _meshGrid;
	_meshGrid = new RegularGrid(_sceneGroup[0]->getAABB(), _fractParameters._gridSubdivisions);
	_meshGrid->fill(_mesh->getModelComponent(0)->_geometry, _mesh->getModelComponent(0)->_topology, _fractParameters._fillShape, 1000, _sceneGPUData[0]);
	//_meshGrid->queryCluster(_mesh->getModelComponent(0)->_geometry, _mesh->getModelComponent(0)->_topology, clusterIdx, 1);
	std::fill(clusterIdx.begin(), clusterIdx.end(), 0);
	_meshGrid->getAABBs(aabbs);

	_aabbRenderer->load(aabbs);
	_aabbRenderer->homogenize();
	_mesh->getModelComponent(0)->setClusterIdx(clusterIdx);
}

void CADScene::recalculateGridSize(ivec3& voxelDimensions, uint8_t lastIndex)
{
	float scale[3];
	AABB aabb = _sceneGroup[0]->getAABB();

	for (int i = 0; i < 3; ++i)
		scale[i] = aabb.extent()[i] / aabb.extent()[lastIndex];

	for (int i = 0; i < 3; ++i)
		voxelDimensions[i] = voxelDimensions[lastIndex] * scale[i];
}

void CADScene::render(const mat4& mModel, RenderingParameters* rendParams)
{
	SSAOScene::render(mModel, rendParams);
}

// [Protected methods]

std::string CADScene::fractureModel()
{
	// Configure seed
	srand(_fractParameters._seed);
	RandomUtilities::initSeed(_fractParameters._seed);

	fracturer::DistanceFunction dfunc = static_cast<fracturer::DistanceFunction>(_fractParameters._distanceFunction);
	
	std::vector<uvec4> seeds;
	std::vector<float> clusterIdx;

	if (_fractParameters._biasSeeds == 0)
	{
		seeds = fracturer::Seeder::uniform(*_meshGrid, _fractParameters._numSeeds, _fractParameters._seedingRandom);
	}
	else
	{
		seeds = fracturer::Seeder::uniform(*_meshGrid, _fractParameters._biasSeeds, _fractParameters._seedingRandom);
		seeds = fracturer::Seeder::nearSeeds(*_meshGrid, seeds, _fractParameters._numSeeds - _fractParameters._biasSeeds, _fractParameters._spreading);
	}

	if (_fractParameters._numExtraSeeds > 0)
	{
		fracturer::DistanceFunction mergeDFunc = static_cast<fracturer::DistanceFunction>(_fractParameters._mergeSeedsDistanceFunction);
		auto extraSeeds = fracturer::Seeder::uniform(*_meshGrid, _fractParameters._numExtraSeeds, _fractParameters._seedingRandom);
		extraSeeds.insert(extraSeeds.begin(), seeds.begin(), seeds.end());

		fracturer::Seeder::mergeSeeds(seeds, extraSeeds, mergeDFunc);
		seeds = extraSeeds;
	}

	ChronoUtilities::initChrono();
	
	if (!_fractParameters._useNaiveVoronoi)
	{
		fracturer::Fracturer* fracturer = nullptr;
		if (_fractParameters._fractureAlgorithm == FractureParameters::NAIVE)
			fracturer = fracturer::NaiveFracturer::getInstance();
		else
			fracturer = fracturer::FloodFracturer::getInstance();

		if (!fracturer->setDistanceFunction(dfunc)) return "Invalid distance function";
		fracturer->build(*_meshGrid, seeds, &_fractParameters);
	}
	else
	{
		std::vector<vec3> seeds3;
		for (const vec4& seed : seeds)
			seeds3.push_back(seed + vec4(.5f));

		Voronoi voronoi(seeds3);
		_meshGrid->fill(voronoi);
	}

	if (_fractParameters._erode)
		_meshGrid->erode(static_cast<FractureParameters::ErosionType>(
			_fractParameters._erosionConvolution), _fractParameters._erosionSize, _fractParameters._erosionIterations,
			_fractParameters._erosionProbability, _fractParameters._erosionThreshold);

	std::cout << ChronoUtilities::getDuration() << std::endl;

	{
		std::vector<AABB> aabbs;
		_meshGrid->getAABBs(aabbs);
		_aabbRenderer->load(aabbs);
	}

	if (_fractParameters._computeMCFragments) _fractureMeshes = _meshGrid->toTriangleMesh();

	_aabbRenderer->setColorIndex(_meshGrid->data(), _meshGrid->getNumSubdivisions().x * _meshGrid->getNumSubdivisions().y * _meshGrid->getNumSubdivisions().z);
	_meshGrid->queryCluster(_mesh->getModelComponent(0)->_geometry, _mesh->getModelComponent(0)->_topology, clusterIdx);
	_mesh->getModelComponent(0)->setClusterIdx(clusterIdx, false);

	return "";
}

void CADScene::loadDefaultCamera(Camera* camera)
{
	AABB aabb = this->getAABB();

	camera->setLookAt(aabb.center());
	camera->setPosition(aabb.center() + aabb.extent() * 1.5f);
}

void CADScene::loadDefaultLights()
{
	Light* pointLight_01 = new Light();
	pointLight_01->setLightType(Light::POINT_LIGHT);
	pointLight_01->setPosition(vec3(1.64f, 2.0f, -0.12f));
	pointLight_01->setId(vec3(0.35f));
	pointLight_01->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(pointLight_01));

	Light* pointLight_02 = new Light();
	pointLight_02->setLightType(Light::POINT_LIGHT);
	pointLight_02->setPosition(vec3(-2.86f, 2.0f, -0.13f));
	pointLight_02->setId(vec3(0.35f));
	pointLight_02->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(pointLight_02));

	Light* sunLight = new Light();
	Camera* camera = sunLight->getCamera();
	ShadowMap* shadowMap = sunLight->getShadowMap();
	camera->setBottomLeftCorner(vec2(-7.0f, -7.0f));
	shadowMap->modifySize(4096, 4096);
	sunLight->setLightType(Light::DIRECTIONAL_LIGHT);
	sunLight->setPosition(vec3(.0f, 3.0f, -5.0f));
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

	if (!this->readCameraFromSettings(camera))
	{
		this->loadDefaultCamera(camera);
	}

	_cameraManager->insertCamera(camera);
}

void CADScene::loadLights()
{
	if (!this->readLightsFromSettings())
	{
		this->loadDefaultLights();
	}

	Scene::loadLights();
}

void CADScene::loadModels()
{
	this->loadModel(MESH_1_PATH);
}

bool CADScene::readCameraFromSettings(Camera* camera)
{
	const std::string filename = SCENE_SETTINGS_FOLDER + SCENE_CAMERA_FILE;
	std::string currentLine, lineHeader;	
	std::stringstream line;
	std::ifstream inputStream;	
	vec3 value;

	inputStream.open(filename.c_str());

	if (inputStream.fail()) return false;

	while (!(inputStream >> std::ws).eof())
	{
		std::getline(inputStream, currentLine);

		line.clear();
		line.str(currentLine);
		std::getline(line, lineHeader, ' ');

		if (lineHeader.find(COMMENT_CHAR) == std::string::npos)
		{
			for (int i = 0; i < 3; ++i)
			{
				line >> value[i];
				line.ignore();
			}

			if (lineHeader == CAMERA_POS_HEADER)
			{
				camera->setPosition(value);
			}
			else if (lineHeader == CAMERA_LOOKAT_HEADER)
			{
				camera->setLookAt(value);
			}
		}
	}

	inputStream.close();	

	return true;
}

bool CADScene::readLightsFromSettings()
{
	// File management
	const std::string filename = SCENE_SETTINGS_FOLDER + SCENE_LIGHTS_FILE;
	std::string currentLine, lineHeader;
	std::stringstream line;
	std::ifstream inputStream;

	Light* light = nullptr;
	vec3 vec3val;
	vec2 vec2val;
	float floatval;
	std::string stringval;

	inputStream.open(filename.c_str());

	if (inputStream.fail()) return false;

	while (!(inputStream >> std::ws).eof())
	{
		std::getline(inputStream, currentLine);

		line.clear();
		line.str(currentLine);
		std::getline(line, lineHeader, '\t');

		if (lineHeader.empty())
		{
			std::getline(line, lineHeader, ' ');
		}

		if (lineHeader.find(COMMENT_CHAR) == std::string::npos)
		{
			if (lineHeader == NEW_LIGHT)
			{
				if (light) _lights.push_back(std::unique_ptr<Light>(light));

				light = new Light();
			}
			else if (light)
			{
				if (lineHeader.find(LIGHT_POSITION) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setPosition(vec3val);
				}
				else if (lineHeader.find(LIGHT_DIRECTION) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setDirection(vec3val);
				}
				else if (lineHeader.find(LIGHT_TYPE) != std::string::npos)
				{
					line >> stringval;

					Light::LightModels type = Light::stringToLightModel(stringval);
					light->setLightType(type);
				}
				else if (lineHeader.find(AMBIENT_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setIa(vec3val);
				}
				else if (lineHeader.find(DIFFUSE_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setId(vec3val);
				}
				else if (lineHeader.find(SPECULAR_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setIs(vec3val);
				}
				else if (lineHeader.find(SHADOW_MAP_SIZE) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->getShadowMap()->modifySize(vec2val.x, vec2val.y);
				}
				else if (lineHeader.find(BLUR_SHADOW_SIZE) != std::string::npos)
				{
					line >> floatval;

					light->setBlurFilterSize(floatval);
				}
				else if (lineHeader.find(ORTHO_SIZE) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->getCamera()->setBottomLeftCorner(vec2val);
				}
				else if (lineHeader.find(SHADOW_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->setShadowIntensity(vec2val.x, vec2val.y);
				}
				else if (lineHeader.find(CAST_SHADOWS) != std::string::npos)
				{
					line >> stringval;

					light->castShadows(stringval == "true" || stringval == "True");
				}
				else if (lineHeader.find(SHADOW_CAMERA_ANGLE_X) != std::string::npos)
				{
					line >> floatval;

					light->getCamera()->setFovX(floatval);
				}
				else if (lineHeader.find(SHADOW_CAMERA_ANGLE_Y) != std::string::npos)
				{
					line >> floatval;

					light->getCamera()->setFovY(floatval);
				}
				else if (lineHeader.find(SHADOW_CAMERA_RASPECT) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->getCamera()->setRaspect(vec2val.x, vec2val.y);
				}
				else if (lineHeader.find(SHADOW_RADIUS) != std::string::npos)
				{
					line >> floatval;

					light->setShadowRadius(floatval);
				}
			}
		}
	}

	if (light) _lights.push_back(std::unique_ptr<Light>(light));

	inputStream.close();

	return true;
}

// [Rendering]

void CADScene::drawAsTriangles(Camera* camera, const mat4& mModel, RenderingParameters* rendParams)
{
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::TRIANGLE_MESH_SHADER);
	RenderingShader* clusteringShader = ShaderList::getInstance()->getRenderingShader(RendEnum::CLUSTER_SHADER);
	RenderingShader* multiInstanceShader = ShaderList::getInstance()->getRenderingShader(RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_SHADER);

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

			if (rendParams->_showVoxelizedMesh)
			{
				multiInstanceShader->use();
				multiInstanceShader->setUniform("materialScattering", rendParams->_materialScattering);
				_lights[i]->applyLight(multiInstanceShader, matrix[RendEnum::VIEW_MATRIX]);
				_lights[i]->applyShadowMapTexture(multiInstanceShader);
				multiInstanceShader->applyActiveSubroutines();

				this->drawSceneAsTriangles(multiInstanceShader, RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_SHADER, &matrix, rendParams);
			}
			else
			{
				if (rendParams->_showTriangleMesh or rendParams->_showFragmentsMarchingCubes)
				{
					shader->use();
					shader->setUniform("materialScattering", rendParams->_materialScattering);
					_lights[i]->applyLight(shader, matrix[RendEnum::VIEW_MATRIX]);
					_lights[i]->applyShadowMapTexture(shader);
					shader->applyActiveSubroutines();

					this->drawSceneAsTriangles(shader, RendEnum::TRIANGLE_MESH_SHADER, &matrix, rendParams);
				}
				else
				{
					clusteringShader->use();
					clusteringShader->setUniform("materialScattering", rendParams->_materialScattering);
					_lights[i]->applyLight(clusteringShader, matrix[RendEnum::VIEW_MATRIX]);
					_lights[i]->applyShadowMapTexture(clusteringShader);
					clusteringShader->applyActiveSubroutines();

					this->drawSceneAsTriangles(clusteringShader, RendEnum::CLUSTER_SHADER, &matrix, rendParams);
				}
			}
		}
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);					// Back to initial state
	glDepthFunc(GL_LESS);
}

void CADScene::drawSceneAsPoints(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
}

void CADScene::drawSceneAsLines(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	for (Group3D* group : _sceneGroup)
		group->drawAsLines(shader, shaderType, *matrix);
}

void CADScene::drawSceneAsTriangles(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_SHADER)
	{
		if (rendParams->_showTriangleMesh)
		{
			for (Group3D* group : _sceneGroup)
			{
				group->drawAsTriangles(shader, shaderType, *matrix);
			}
		}
		else if (rendParams->_showFragmentsMarchingCubes)
		{
			for (Model3D* fractureMesh : _fractureMeshes)
				fractureMesh->drawAsTriangles(shader, shaderType, *matrix);
		}
	}
	else if (shaderType == RendEnum::CLUSTER_SHADER)
	{
		if (!rendParams->_showFragmentsMarchingCubes)
		{
			for (Group3D* group : _sceneGroup)
				group->drawAsTriangles(shader, shaderType, *matrix);
		}
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
		if (!rendParams->_showVoxelizedMesh)
		{
			if (rendParams->_showTriangleMesh or !rendParams->_showFragmentsMarchingCubes)
			{
				for (Group3D* group : _sceneGroup)
					group->drawAsTriangles4Shadows(shader, shaderType, *matrix);
			}
			else if (rendParams->_showFragmentsMarchingCubes)
			{
				for (Model3D* fractureMesh : _fractureMeshes)
					fractureMesh->drawAsTriangles4Shadows(shader, shaderType, *matrix);
			}
		}
	}
	else
	{
		if (rendParams->_showVoxelizedMesh)
		{
			if (rendParams->_planeClipping)
			{
				shader->setUniform("planeCoefficients", rendParams->_planeCoefficients);
			}

			_aabbRenderer->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
	}
}

void CADScene::drawSceneAsTriangles4Position(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_POSITION_SHADER || shaderType == RendEnum::SHADOWS_SHADER)
	{
		if (!rendParams->_showVoxelizedMesh)
		{
			if (rendParams->_showTriangleMesh or !rendParams->_showFragmentsMarchingCubes)
			{
				for (Group3D* group : _sceneGroup)
					group->drawAsTriangles4Shadows(shader, shaderType, *matrix);
			}
			else if (rendParams->_showFragmentsMarchingCubes)
			{
				for (Model3D* fractureMesh : _fractureMeshes)
					fractureMesh->drawAsTriangles4Shadows(shader, shaderType, *matrix);
			}
		}
	}
	else
	{
		if (rendParams->_showVoxelizedMesh)
		{
			if (rendParams->_planeClipping)
			{
				shader->setUniform("planeCoefficients", rendParams->_planeCoefficients);
			}

			_aabbRenderer->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
	}
}