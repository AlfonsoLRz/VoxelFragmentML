#include "stdafx.h"
#include "CADModel.h"

#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Face_count_stop_predicate.h>
#include "CGALInterface.h"
#include "Graphics/Application/MaterialList.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/VAO.h"
#include "Simplify.h"
#include "Utilities/FileManagement.h"
#include "Utilities/ChronoUtilities.h"

// Initialization of static attributes
std::unordered_map<std::string, std::unique_ptr<Material>> CADModel::_cadMaterials;
std::unordered_map<std::string, std::unique_ptr<Texture>> CADModel::_cadTextures;

const std::string CADModel::BINARY_EXTENSION = ".bin";
const float CADModel::MODEL_NORMALIZATION_SCALE = 5.0f;

/// [Public methods]

CADModel::CADModel(const std::string& filename, const bool useBinary, const bool mergeVertices) :
	Model3D(mat4(1.0f), 0), _scene(nullptr)
{
	_filename = filename;
	_fuseVertices = mergeVertices;
	_useBinary = useBinary;
}

CADModel::CADModel(const std::vector<Triangle3D>& triangles, bool releaseMemory, const mat4& modelMatrix) :
	Model3D(modelMatrix, 1), _useBinary(false), _fuseVertices(false), _scene(nullptr)
{
	for (const Triangle3D& triangle : triangles)
	{
		vec3 normal = triangle.normal();
		for (int i = 0; i < 3; ++i)
		{
			_modelComp[0]->_geometry.push_back(Model3D::VertexGPUData{ triangle.getPoint(i), .0f, normal });
		}
		_modelComp[0]->_topology.push_back(Model3D::FaceGPUData{ uvec3(_modelComp[0]->_geometry.size() - 3, _modelComp[0]->_geometry.size() - 2, _modelComp[0]->_geometry.size() - 1) });
	}

	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->buildTriangleMeshTopology();
	}

	Model3D::setVAOData();

	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->releaseMemory(releaseMemory, releaseMemory, releaseMemory);
	}
}

CADModel::CADModel(const mat4& modelMatrix) : Model3D(modelMatrix, 1), _useBinary(false), _fuseVertices(false), _scene(nullptr)
{
}

CADModel::~CADModel()
{
}

void CADModel::endInsertionBatch(bool releaseMemory)
{
#if !GENERATE_DATASET
	for (ModelComponent* modelComponent : _modelComp)
	{
		this->computeMeshData(modelComponent);

		modelComponent->buildTriangleMeshTopology();
		modelComponent->buildWireframeTopology();
		modelComponent->buildPointCloudTopology();
	}

	this->setVAOData();
#endif

	for (ModelComponent* modelComponent : _modelComp)
		modelComponent->releaseMemory(releaseMemory, true, releaseMemory);
}

std::string CADModel::getShortName() const
{
	const size_t barPosition = _filename.find_last_of('/');
	const size_t dotPosition = _filename.find_last_of('.');

	return _filename.substr(barPosition + 1, dotPosition - barPosition - 1);
}

void CADModel::insert(vec4* vertices, unsigned numVertices, uvec4* faces, unsigned numFaces, bool updateIndices)
{
	ModelComponent* modelComponent = _modelComp[0];

	unsigned baseGeometryIndex = modelComponent->_geometry.size();
	modelComponent->_geometry.resize(baseGeometryIndex + numVertices);
#pragma omp parallel for
	for (int idx = 0; idx < numVertices; ++idx)
		modelComponent->_geometry[baseGeometryIndex + idx] = Model3D::VertexGPUData{ vec3(vertices[idx].x, vertices[idx].y, vertices[idx].z) };

	unsigned baseTopologyIndex = modelComponent->_topology.size();
	modelComponent->_topology.resize(modelComponent->_topology.size() + numFaces);
#pragma omp parallel for
	for (int idx = 0; idx < numFaces; ++idx)
		modelComponent->_topology[baseTopologyIndex + idx] =
		Model3D::FaceGPUData{
			uvec3(faces[idx].x + baseGeometryIndex, faces[idx].y + baseGeometryIndex, faces[idx].z + baseGeometryIndex) };
}

bool CADModel::load()
{
	std::string binaryFile = _filename.substr(0, _filename.find_last_of('.')) + BINARY_EXTENSION;

	if ((_useBinary && !GENERATE_DATASET) && std::filesystem::exists(binaryFile))
	{
		this->loadModelFromBinaryFile(binaryFile);
	}
	else
	{
		_scene = _assimpImporter.ReadFile(_filename, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals);

		if (!_scene || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << _assimpImporter.GetErrorString() << std::endl;
			return this;
		}

		std::string shortName = _scene->GetShortFilename(_filename.c_str());
		std::string folder = _filename.substr(0, _filename.length() - shortName.length());

		this->processNode(_scene->mRootNode, _scene, folder);
		this->fuseComponents();

		if (_fuseVertices)
		{
			for (Model3D::ModelComponent* modelComp : _modelComp)
			{
				std::vector<int> mapping(_modelComp[0]->_geometry.size());
				std::iota(mapping.begin(), mapping.end(), 0);

				this->fuseVertices(mapping);
				this->remapVertices(_modelComp[0], mapping);
			}
		}

#if GENERATE_DATASET
		glm::vec3 scale = _aabb.extent();
		float maxScale = MODEL_NORMALIZATION_SCALE / glm::max(scale.x, glm::max(scale.y, scale.z)) * 2.0f;

		const glm::mat4 transformation = glm::scale(glm::mat4(1.0f), glm::vec3(maxScale)) * glm::translate(glm::mat4(1.0f), -_aabb.center());
		this->transformGeometry(transformation);

		_aabb.dot(transformation);
		for (Model3D::ModelComponent* modelComp : _modelComp)
			modelComp->_aabb.dot(transformation);
#endif

		std::cout << "Number of vertices: " << this->getNumVertices() << std::endl;
		std::cout << "Number of faces: " << this->getNumFaces() << std::endl;

		this->writeBinary(binaryFile);
	}

	for (ModelComponent* modelComponent : _modelComp)
	{
#if !GENERATE_DATASET
		modelComponent->buildTriangleMeshTopology();
		modelComponent->buildPointCloudTopology();
		modelComponent->buildWireframeTopology();
#endif
	}

#if !GENERATE_DATASET
	this->setVAOData();
#endif

	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->updateSSBO();
		modelComponent->releaseMemory(false, true, false);
	}

	return true;
}

PointCloud3D* CADModel::sample(unsigned maxSamples, int randomFunction)
{
	PointCloud3D* pointCloud = nullptr;

	if (!_modelComp.empty())
	{
		Model3D::ModelComponent* component = _modelComp[0];
		pointCloud = new PointCloud3D;

		// Other GPU buffers
		unsigned numPoints = 0;
		const GLuint pointCloudSSBO = ComputeShader::setWriteBuffer(vec4(), maxSamples * 1.2f, GL_DYNAMIC_DRAW);
		const GLuint countingSSBO = ComputeShader::setReadBuffer(&numPoints, 1, GL_DYNAMIC_DRAW);

		// Max. triangle area
		float sumArea = 0.0f, maxArea = .0f;
		std::vector<float> triangleArea(component->_topology.size());
		this->getSortedTriangleAreas(component, triangleArea, sumArea, maxArea);

		// Noise to generate randomized points within each triangle
		std::vector<float> noiseBuffer;
		unsigned noiseBufferSize = glm::ceil(maxArea / sumArea * maxSamples);
		fracturer::Seeder::getFloatNoise(noiseBufferSize, noiseBufferSize * 2, randomFunction, noiseBuffer);
		const GLuint noiseSSBO = ComputeShader::setReadBuffer(noiseBuffer, GL_STATIC_DRAW);

		if (maxSamples > component->_topology.size())
		{
			ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::SAMPLER_SHADER);

			shader->use();
			shader->bindBuffers(std::vector<GLuint> { component->_geometrySSBO, component->_topologySSBO, pointCloudSSBO, countingSSBO, noiseSSBO });
			shader->setUniform("numSamples", maxSamples);
			shader->setUniform("numTriangles", static_cast<unsigned>(component->_topology.size()));
			shader->setUniform("sumArea", sumArea);
			shader->execute(ComputeShader::getNumGroups(component->_topology.size()), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

			numPoints = *ComputeShader::readData(countingSSBO, unsigned());
			vec4* pointCloudData = ComputeShader::readData(pointCloudSSBO, vec4());
			pointCloud->push_back(pointCloudData, numPoints);
			pointCloud->subselect(maxSamples);
		}
		else
		{
			ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::SAMPLER_ALT_SHADER);

			int activeFaces = 0, randomFace;
			std::vector<uint8_t> activeBuffer(component->_topology.size(), uint8_t(0));
			while (activeFaces < maxSamples)
			{
				randomFace = RandomUtilities::getUniformRandomInt(0, component->_topology.size() - 1);
				if (activeBuffer[randomFace] == 0)
				{
					activeBuffer[randomFace] = 1;
					++activeFaces;
				}
			}

			const GLuint activeSSBO = ComputeShader::setReadBuffer(activeBuffer, GL_STATIC_DRAW);

			shader->use();
			shader->bindBuffers(std::vector<GLuint> { component->_geometrySSBO, component->_topologySSBO, pointCloudSSBO, countingSSBO, noiseSSBO, activeSSBO });
			shader->setUniform("noiseBufferSize", maxSamples * 2);
			shader->setUniform("numTriangles", static_cast<unsigned>(component->_topology.size()));
			shader->execute(ComputeShader::getNumGroups(component->_topology.size()), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

			numPoints = *ComputeShader::readData(countingSSBO, unsigned());
			vec4* pointCloudData = ComputeShader::readData(pointCloudSSBO, vec4());
			pointCloud->push_back(pointCloudData, numPoints);

			ComputeShader::deleteBuffers(std::vector<GLuint>{ activeSSBO });
		}

		ComputeShader::deleteBuffers(std::vector<GLuint>{ pointCloudSSBO, countingSSBO, noiseSSBO });
	}

	return pointCloud;
}

PointCloud3D* CADModel::sampleCPU(unsigned maxSamples, int randomFunction)
{
	PointCloud3D* pointCloud = nullptr;

	if (!_modelComp.empty())
	{
		Model3D::ModelComponent* component = _modelComp[0];
		pointCloud = new PointCloud3D;

		unsigned numPoints = 0;
		float sumArea = 0.0f, maxArea = .0f;
		std::vector<float> triangleArea(component->_topology.size());
		this->getSortedTriangleAreas(component, triangleArea, sumArea, maxArea);

		// Noise to generate randomized points within each triangle
		std::vector<float> noiseBuffer;
		unsigned noiseBufferSize = glm::max(1, static_cast<int>(glm::ceil(maxArea / sumArea * maxSamples)));
		fracturer::Seeder::getFloatNoise(noiseBufferSize, noiseBufferSize * 2, randomFunction, noiseBuffer);
		noiseBufferSize *= 2;

		// Data in common for the two branchs
		int pointIndex = 0;

		if (maxSamples > component->_topology.size())
		{
			std::vector<vec4> newPoint(maxSamples * 2);

			#pragma omp parallel for
			for (int index = 0; index < component->_topology.size(); ++index)
			{
				glm:: vec3 v1 = component->_geometry[component->_topology[index]._vertices.x]._position,
						   v2 = component->_geometry[component->_topology[index]._vertices.y]._position,
						   v3 = component->_geometry[component->_topology[index]._vertices.z]._position;
				vec3 u = v2 - v1, v = v3 - v1;
				float area = glm::length(glm::cross(u, v)) / 2.0f;
				glm::uint numTriangleSamples = std::max(int(glm::ceil(area / sumArea * maxSamples)), 1), newPointIndex, end;

				#pragma omp critical
				{
					newPointIndex = pointIndex;
					pointIndex += numTriangleSamples;
				}

				end = newPointIndex + numTriangleSamples;

				for (int i = newPointIndex; i < end; i++)
				{
					vec2 randomFactors = vec2(noiseBuffer[i % noiseBufferSize], noiseBuffer[(i + 1) % noiseBufferSize]);
					if ((randomFactors.x + randomFactors.y) >= 1.0f)
						randomFactors = 1.0f - randomFactors;

					vec3 point = v1 + u * randomFactors.x + v * randomFactors.y;
					newPoint[i] = vec4(point, 1.0f);
				}
			}

			newPoint.resize(pointIndex);
			pointCloud->push_back(newPoint.data(), newPoint.size());
			if (pointIndex > maxSamples) pointCloud->subselect(maxSamples);
		}
		else
		{
			std::vector<vec4> newPoint(maxSamples);

			// Randomly select which faces are active
			int activeFaces = 0, randomFace;
			std::vector<uint8_t> activeBuffer(component->_topology.size(), uint8_t(0));
			while (activeFaces < maxSamples)
			{
				randomFace = RandomUtilities::getUniformRandomInt(0, component->_topology.size() - 1);
				if (activeBuffer[randomFace] == 0)
				{
					activeBuffer[randomFace] = 1;
					++activeFaces;
				}
			}

			#pragma omp parallel for
			for (int index = 0; index < component->_topology.size(); ++index)
			{
				if (activeBuffer[index] != uint8_t(0))
				{
					vec3 v1 = component->_geometry[component->_topology[index]._vertices.x]._position,
						v2 = component->_geometry[component->_topology[index]._vertices.y]._position,
						v3 = component->_geometry[component->_topology[index]._vertices.z]._position;
					vec3 u = v2 - v1, v = v3 - v1;
					glm::uint newPointIndex;

					#pragma omp critical
					newPointIndex = pointIndex++;

					vec2 randomFactors = vec2(noiseBuffer[index % noiseBufferSize], noiseBuffer[(index + 1) % noiseBufferSize]);
					if (randomFactors.x + randomFactors.y >= 1.0f)
						randomFactors = 1.0f - randomFactors;

					vec3 point = v1 + u * randomFactors.x + v * randomFactors.y;
					newPoint[newPointIndex] = vec4(point, 1.0f);
				}
			}

			pointCloud->push_back(newPoint.data(), newPoint.size());
		}
	}

	return pointCloud;
}

std::thread* CADModel::save(const std::string& filename, FractureParameters::ExportMeshExtension meshExtension)
{
	const std::string meshExtensionStr = FractureParameters::ExportMesh_STR[meshExtension];
	std::thread* thread = nullptr;
	Model3D::ModelComponent* component = _modelComp[0]->copyComponent(!TESTING_FORMAT_MODE);

	if (meshExtension == FractureParameters::ExportMeshExtension::BINARY_MESH)
		thread = new std::thread(&CADModel::saveBinary, this, filename + "." + meshExtensionStr, component);
	else
		thread = new std::thread(&CADModel::saveAssimp, this, filename + "." + meshExtensionStr, meshExtensionStr, component);

	return thread;
}

void CADModel::simplify(unsigned numFaces, bool verbose)
{
	for (Model3D::ModelComponent* modelComponent : _modelComp)
	{
		if (modelComponent->_topology.size() > numFaces)
		{
			Simplify::vertices.resize(modelComponent->_geometry.size());
			Simplify::triangles.resize(modelComponent->_topology.size());

			#pragma omp parallel for
			for (int i = 0; i < modelComponent->_geometry.size(); ++i)
				Simplify::vertices[i] = Simplify::Vertex{
					vec3f(modelComponent->_geometry[i]._position.x, modelComponent->_geometry[i]._position.y, modelComponent->_geometry[i]._position.z)
			};

			#pragma omp parallel for
			for (int i = 0; i < modelComponent->_topology.size(); ++i)
			{
				Simplify::triangles[i] = Simplify::Triangle();
				for (int j = 0; j < 3; ++j)
					Simplify::triangles[i].v[j] = modelComponent->_topology[i]._vertices[j];
			}

			Simplify::simplify_mesh(numFaces, 5.0);

			modelComponent->_geometry.resize(Simplify::vertices.size());
			modelComponent->_topology.resize(Simplify::triangles.size());

			#pragma omp parallel for
			for (int i = 0; i < Simplify::vertices.size(); ++i)
				modelComponent->_geometry[i]._position = vec3(Simplify::vertices[i].p.x, Simplify::vertices[i].p.y, Simplify::vertices[i].p.z);

			#pragma omp parallel for
			for (int i = 0; i < Simplify::triangles.size(); ++i)
			{
				modelComponent->_topology[i]._vertices = uvec3(Simplify::triangles[i].v[0], Simplify::triangles[i].v[1], Simplify::triangles[i].v[2]);
			}
		}

		if (verbose)
			std::cout << "Simplified to " << modelComponent->_topology.size() << " faces." << std::endl;
	}
}

bool CADModel::subdivide(float maxArea)
{
	bool applyChanges = false;
	std::vector<unsigned> faces;

	#pragma omp parallel for
	for (int modelCompIdx = 0; modelCompIdx < _modelComp.size(); ++modelCompIdx)
	{
		Model3D::ModelComponent* modelComp = _modelComp[modelCompIdx];

		#pragma omp critical
		applyChanges &= modelComp->subdivide(maxArea, faces);
	}

	return applyChanges;
}

void CADModel::transformGeometry(const mat4& mMatrix)
{
	for (ModelComponent* modelComponent : _modelComp)
	{
		#pragma omp parallel for
		for (int idx = 0; idx < modelComponent->_geometry.size(); ++idx)
		{
			modelComponent->_geometry[idx]._position = vec3(mMatrix * vec4(modelComponent->_geometry[idx]._position, 1.0f));
			modelComponent->_geometry[idx]._normal = vec3(mMatrix * vec4(modelComponent->_geometry[idx]._normal, 0.0f));
		}
	}
}

/// [Protected methods]

void CADModel::computeMeshData(ModelComponent* component)
{
	#pragma omp parallel for
	for (int faceIdx = 0; faceIdx < component->_topology.size(); ++faceIdx)
	{
		glm::vec3 v1 = component->_geometry[component->_topology[faceIdx]._vertices.x]._position,
			v2 = component->_geometry[component->_topology[faceIdx]._vertices.y]._position,
			v3 = component->_geometry[component->_topology[faceIdx]._vertices.z]._position;
		glm::vec3 u = v2 - v1, v = v3 - v1;
		v1 = glm::normalize(glm::cross(u, v));

		for (int i = 0; i < 3; ++i)
		{
			#pragma omp critical
			{
				component->_geometry[component->_topology[faceIdx]._vertices[i]]._normal += v1;
				component->_geometry[component->_topology[faceIdx]._vertices[i]]._padding1 += 1.0f;
			}
		}
	}

	#pragma omp parallel for
	for (int vertexIdx = 0; vertexIdx < component->_geometry.size(); ++vertexIdx)
	{
		component->_geometry[vertexIdx]._normal /= component->_geometry[vertexIdx]._padding1;
		component->_geometry[vertexIdx]._normal = glm::normalize(component->_geometry[vertexIdx]._normal);
	}
}

Material* CADModel::createMaterial(ModelComponent* modelComp)
{
	static const std::string nullMaterialName = "None";

	Material* material = MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_CAD_WHITE);
	Material::MaterialDescription& materialDescription = modelComp->_materialDescription;
	std::string name = std::string(materialDescription._name);
	std::string mapKd = std::string(materialDescription._textureImage[Texture::KAD_TEXTURE]);
	std::string mapKs = std::string(materialDescription._textureImage[Texture::KS_TEXTURE]);

	if (!name.empty() && name != nullMaterialName)
	{
		auto itMaterial = _cadMaterials.find(name);

		if (itMaterial == _cadMaterials.end())
		{
			material = new Material();
			Texture* kad, * ks;

			if (!mapKd.empty())
			{
				kad = new Texture(mapKd);
			}
			else
			{
				kad = new Texture(materialDescription._textureColor[Texture::KAD_TEXTURE]);
			}

			if (!mapKs.empty())
			{
				ks = new Texture(mapKs);
			}
			else
			{
				ks = new Texture(materialDescription._textureColor[Texture::KS_TEXTURE]);
				material->setShininess(materialDescription._ns);
			}

			material->setTexture(Texture::KAD_TEXTURE, kad);
			material->setTexture(Texture::KS_TEXTURE, ks);

			_cadMaterials[name] = std::unique_ptr<Material>(material);
			_cadTextures[name + "-kad"] = std::unique_ptr<Texture>(kad);
			_cadTextures[name + "-ks"] = std::unique_ptr<Texture>(ks);
		}
		else
		{
			material = itMaterial->second.get();
		}
	}

	return material;
}

void CADModel::fuseComponents()
{
	if (!_modelComp.size() <= 1)
		return;

	Model3D::ModelComponent* newModelComponent = new Model3D::ModelComponent();
	unsigned numVertices = 0;

	for (Model3D::ModelComponent* modelComponent : _modelComp)
	{
		newModelComponent->_geometry.insert(newModelComponent->_geometry.end(), modelComponent->_geometry.begin(), modelComponent->_geometry.end());
		for (FaceGPUData& face : modelComponent->_topology)
			face._vertices += numVertices;
		newModelComponent->_topology.insert(newModelComponent->_topology.end(), modelComponent->_topology.begin(), modelComponent->_topology.end());

		numVertices += modelComponent->_geometry.size();

		delete modelComponent;
	}

	_modelComp.clear();
	_modelComp.push_back(newModelComponent);
}

void CADModel::fuseVertices(std::vector<int>& mapping)
{
	Model3D::ModelComponent* modelComp = _modelComp[0];

	for (unsigned vertexIdx = 0; vertexIdx < modelComp->_geometry.size(); ++vertexIdx)
	{
		if (mapping[vertexIdx] != vertexIdx)
			continue;

#pragma omp parallel for
		for (int j = vertexIdx + 1; j < modelComp->_geometry.size(); ++j)
		{
			if (mapping[vertexIdx] == vertexIdx && glm::distance(modelComp->_geometry[vertexIdx]._position, modelComp->_geometry[j]._position) < glm::epsilon<float>())
			{
				// Remap
				mapping[j] = vertexIdx;
			}
		}
	}
}

bool CADModel::loadModelFromBinaryFile(const std::string& binaryFile)
{
	bool success;

	if (success = this->readBinary(binaryFile, _modelComp))
	{
		for (ModelComponent* modelComp : _modelComp)
		{
			modelComp->_material = this->createMaterial(modelComp);
			modelComp->setName(modelComp->_name);
		}
	}

	return success;
}

Model3D::ModelComponent* CADModel::processMesh(aiMesh* mesh, const aiScene* scene, const std::string& folder)
{
	std::vector<Model3D::VertexGPUData> vertices(mesh->mNumVertices);
	std::vector<Model3D::FaceGPUData> faces(mesh->mNumFaces);
	AABB aabb;
	Material* material = nullptr;
	Material::MaterialDescription description;
	aiMaterial* aiMaterial = nullptr;

	// Vertices
	int numVertices = static_cast<int>(mesh->mNumVertices);

	#pragma omp parallel for
	for (int i = 0; i < numVertices; i++)
	{
		Model3D::VertexGPUData vertex;
		vertex._position = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		vertex._normal = vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

		if (mesh->mTangents) vertex._tangent = vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		if (mesh->mTextureCoords[0]) vertex._textCoord = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

		vertices[i] = vertex;

#pragma omp critical
		aabb.update(vertex._position);
	}

	// Indices
#pragma omp parallel for
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace& face = mesh->mFaces[i];
		faces[i] = Model3D::FaceGPUData{ uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]) };
	}

	// Material
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
		std::string materialName = aiMaterial->GetName().C_Str();
		MaterialList* materialList = MaterialList::getInstance();

		description = std::move(Material::getMaterialDescription(aiMaterial, folder));
	}

	ModelComponent* component = new ModelComponent;
	component->_geometry = std::move(vertices);
	component->_topology = std::move(faces);
	component->_aabb = std::move(aabb);
	component->_materialDescription = description;
	component->_material = createMaterial(component);
	component->_name = component->_materialDescription._name;

	return component;
}

void CADModel::processNode(aiNode* node, const aiScene* scene, const std::string& folder)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		_modelComp.push_back(this->processMesh(mesh, scene, folder));
		_aabb.update(_modelComp[_modelComp.size() - 1]->_aabb);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		this->processNode(node->mChildren[i], scene, folder);
	}
}

bool CADModel::readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp)
{
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	if (!fin.is_open()) return false;

	size_t numModelComps, numVertices, numTriangles, numIndices;

	fin.read((char*)&numModelComps, sizeof(size_t));
	while (_modelComp.size() < numModelComps)
	{
		_modelComp.push_back(new ModelComponent());
	}

	for (Model3D::ModelComponent* component : modelComp)
	{
		fin.read((char*)&numVertices, sizeof(size_t));
		component->_geometry.resize(numVertices);
		if (numVertices)
			fin.read((char*)&component->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

		fin.read((char*)&numTriangles, sizeof(size_t));
		component->_topology.resize(numTriangles);
		if (numTriangles)
			fin.read((char*)&component->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

		fin.read((char*)&numIndices, sizeof(size_t));
		component->_triangleMesh.resize(numIndices);
		if (numIndices)
			fin.read((char*)&component->_triangleMesh[0], numIndices * sizeof(GLuint));

		fin.read((char*)&numIndices, sizeof(size_t));
		component->_pointCloud.resize(numIndices);
		if (numIndices)
			fin.read((char*)&component->_pointCloud[0], numIndices * sizeof(GLuint));

		fin.read((char*)&numIndices, sizeof(size_t));
		component->_wireframe.resize(numIndices);
		if (numIndices) 
			fin.read((char*)&component->_wireframe[0], numIndices * sizeof(GLuint));

		size_t length;
		fin.read((char*)&length, sizeof(size_t));
		component->_name.resize(length);
		if (length) 
			fin.read((char*)&component->_name[0], length);

		fin.read((char*)&component->_aabb, sizeof(AABB));

		// Recover material description
		fin.read((char*)&length, sizeof(size_t));
		component->_materialDescription._rootFolder.resize(length);
		if (length) 
			fin.read((char*)&component->_materialDescription._rootFolder[0], length);

		fin.read((char*)&length, sizeof(size_t));
		component->_materialDescription._name.resize(length);
		if (length) 
			fin.read((char*)&component->_materialDescription._name[0], length);

		for (int textureLayer = 0; textureLayer < Texture::NUM_TEXTURE_TYPES; textureLayer += 1)
		{
			fin.read((char*)&length, sizeof(size_t));
			component->_materialDescription._textureImage[textureLayer].resize(length);
			if (length) 
				fin.read((char*)&component->_materialDescription._textureImage[textureLayer][0], length);
			fin.read((char*)&component->_materialDescription._textureColor[textureLayer], sizeof(vec4));
		}
		fin.read((char*)&component->_materialDescription._ns, sizeof(float));

		component->_material = this->createMaterial(component);
	}

	fin.read((char*)&_aabb, sizeof(AABB));
	fin.close();

	return true;
}

void CADModel::remapVertices(Model3D::ModelComponent* modelComponent, std::vector<int>& mapping)
{
	unsigned erasedVertices = 0, startingSize = modelComponent->_geometry.size();
	std::vector<unsigned> newMapping(startingSize);

	for (size_t vertexIdx = 0; vertexIdx < startingSize; ++vertexIdx)
	{
		newMapping[vertexIdx] = vertexIdx - erasedVertices;

		if (mapping[vertexIdx] != vertexIdx)
		{
			modelComponent->_geometry.erase(modelComponent->_geometry.begin() + vertexIdx - erasedVertices);
			++erasedVertices;
		}
	}

	for (FaceGPUData& face : modelComponent->_topology)
	{
		for (int i = 0; i < 3; ++i)
		{
			face._vertices[i] = newMapping[mapping[face._vertices[i]]];
		}
	}
}

void CADModel::saveAssimp(const std::string& filename, const std::string& extension, Model3D::ModelComponent* component)
{
	aiScene* scene = new aiScene;
	scene->mRootNode = new aiNode();

	scene->mMaterials = new aiMaterial * [1];
	scene->mMaterials[0] = nullptr;
	scene->mNumMaterials = 1;

	scene->mMaterials[0] = new aiMaterial();

	scene->mMeshes = new aiMesh * [1];
	scene->mMeshes[0] = nullptr;
	scene->mNumMeshes = 1;

	scene->mMeshes[0] = new aiMesh();
	scene->mMeshes[0]->mMaterialIndex = 0;

	scene->mRootNode->mMeshes = new unsigned int[1];
	scene->mRootNode->mMeshes[0] = 0;
	scene->mRootNode->mNumMeshes = 1;

	auto pMesh = scene->mMeshes[0];

	// Retrieving info from the model
	std::vector<glm::vec3> vertices;
	std::vector<ivec3> faces;

	for (VertexGPUData& vertex : component->_geometry)
		vertices.push_back(vertex._position);

	for (FaceGPUData& face : component->_topology)
		faces.push_back(ivec3(face._vertices.x, face._vertices.y, face._vertices.z));

	// Vertex generation
	const auto& assimpVertices = vertices;

	pMesh->mVertices = new aiVector3D[assimpVertices.size()];
	pMesh->mNumVertices = assimpVertices.size();

	int j = 0;
	for (auto itr = assimpVertices.begin(); itr != assimpVertices.end(); ++itr)
	{
		pMesh->mVertices[itr - assimpVertices.begin()] = aiVector3D(assimpVertices[j].x, assimpVertices[j].y, assimpVertices[j].z);
		++j;
	}

	// Index generation
	pMesh->mFaces = new aiFace[faces.size()];
	pMesh->mNumFaces = static_cast<unsigned>(faces.size());

	for (size_t i = 0; i < faces.size(); i++)
	{
		aiFace& face = pMesh->mFaces[i];
		face.mIndices = new unsigned int[3];
		face.mNumIndices = 3;
		face.mIndices[0] = faces[i].x;
		face.mIndices[1] = faces[i].y;
		face.mIndices[2] = faces[i].z;
	}

	Assimp::Exporter exporter;
	exporter.Export(scene, extension, filename);

	delete component;
	delete scene;
}

void CADModel::saveBinary(const std::string& filename, Model3D::ModelComponent* component)
{
	std::ofstream fout(filename, std::ios::out | std::ios::binary);
	if (!fout.is_open())
		return;

	const size_t numModelComps = _modelComp.size();
	fout.write((char*)&numModelComps, sizeof(size_t));

	const size_t numVertices = component->_geometry.size();
	fout.write((char*)&numVertices, sizeof(size_t));
	if (numVertices) fout.write((char*)&component->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

	const size_t numTriangles = component->_topology.size();
	fout.write((char*)&numTriangles, sizeof(size_t));
	if (numTriangles) fout.write((char*)&component->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

	delete component;
	fout.close();
}

bool CADModel::writeBinary(const std::string& path)
{
	std::ofstream fout(path, std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		return false;
	}

	size_t numIndices;
	const size_t numModelComps = _modelComp.size();
	fout.write((char*)&numModelComps, sizeof(size_t));

	for (Model3D::ModelComponent* component : _modelComp)
	{
		const size_t numVertices = component->_geometry.size();
		fout.write((char*)&numVertices, sizeof(size_t));
		if (numVertices) fout.write((char*)&component->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

		const size_t numTriangles = component->_topology.size();
		fout.write((char*)&numTriangles, sizeof(size_t));
		if (numTriangles) fout.write((char*)&component->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

		numIndices = component->_triangleMesh.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		if (numIndices) fout.write((char*)&component->_triangleMesh[0], numIndices * sizeof(GLuint));

		numIndices = component->_pointCloud.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		if (numIndices) fout.write((char*)&component->_pointCloud[0], numIndices * sizeof(GLuint));

		numIndices = component->_wireframe.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		if (numIndices) fout.write((char*)&component->_wireframe[0], numIndices * sizeof(GLuint));

		size_t nameLength = component->_name.size();
		fout.write((char*)&nameLength, sizeof(size_t));
		if (nameLength) fout.write((char*)&component->_name[0], nameLength);

		fout.write((char*)(&component->_aabb), sizeof(AABB));

		// Write material description
		size_t rootFolderLength = component->_materialDescription._rootFolder.size(), length;
		fout.write((char*)&rootFolderLength, sizeof(size_t));
		if (rootFolderLength) fout.write((char*)&component->_materialDescription._rootFolder[0], rootFolderLength);

		nameLength = component->_materialDescription._name.size();
		fout.write((char*)&nameLength, sizeof(size_t));
		if (nameLength) fout.write((char*)&component->_materialDescription._name[0], nameLength);

		for (int textureLayer = 0; textureLayer < Texture::NUM_TEXTURE_TYPES; textureLayer += 1)
		{
			length = component->_materialDescription._textureImage[textureLayer].size();
			fout.write((char*)&length, sizeof(size_t));
			if (length) fout.write((char*)&component->_materialDescription._textureImage[textureLayer][0], length);
			fout.write((char*)&component->_materialDescription._textureColor[textureLayer], sizeof(vec4));
		}
		fout.write((char*)&component->_materialDescription._ns, sizeof(float));
	}

	fout.write((char*)&_aabb, sizeof(AABB));

	fout.close();

	return true;
}