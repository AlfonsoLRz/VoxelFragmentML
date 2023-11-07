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

/// [Public methods]

CADModel::CADModel(const std::string& filename, const bool useBinary, const bool fuseComponents, const bool mergeVertices) :
	Model3D(mat4(1.0f), 0)
{
	_filename = filename;
	_fuseComponents = fuseComponents;
	_fuseVertices = mergeVertices;
	_useBinary = useBinary;
}

CADModel::CADModel(const std::vector<Triangle3D>& triangles, bool releaseMemory, const mat4& modelMatrix) :
	Model3D(modelMatrix, 1), _useBinary(false), _fuseComponents(false), _fuseVertices(false), _scene(nullptr)
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
		modelComponent->releaseMemory(releaseMemory, releaseMemory);
	}
}

CADModel::CADModel(const mat4& modelMatrix) : Model3D(modelMatrix, 1), _useBinary(false), _fuseComponents(false), _fuseVertices(false), _scene(nullptr)
{
}

CADModel::~CADModel()
{
	delete _scene;
}

void CADModel::endInsertionBatch(bool releaseMemory, bool buildVao, int targetFaces)
{
	ModelComponent* modelComponent = _modelComp[0];

	if (targetFaces > 0 and targetFaces < this->getNumFaces())
		this->simplify(targetFaces);

	if (buildVao)
	{
		this->computeMeshData(modelComponent, true);

		for (ModelComponent* modelComponent : _modelComp)
		{
			modelComponent->buildTriangleMeshTopology();
#if !DATASET_GENERATION
			modelComponent->buildWireframeTopology();
			modelComponent->buildPointCloudTopology();
#endif
		}

		this->setVAOData();
	}

	for (ModelComponent* modelComponent : _modelComp)
		modelComponent->releaseMemory(releaseMemory, releaseMemory);
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
	unsigned baseIndex = modelComponent->_geometry.size();

	for (int idx = 0; idx < numVertices; ++idx)
		modelComponent->_geometry.push_back(Model3D::VertexGPUData{ vec3(vertices[idx].x, vertices[idx].y, vertices[idx].z) });

	for (int idx = 0; idx < numFaces; ++idx)
		modelComponent->_topology.push_back(Model3D::FaceGPUData{ uvec3(faces[idx].x + baseIndex, faces[idx].y + baseIndex, faces[idx].z + baseIndex) });
}

bool CADModel::load()
{
	std::string binaryFile = _filename.substr(0, _filename.find_last_of('.')) + BINARY_EXTENSION;

	if (_useBinary && std::filesystem::exists(binaryFile))
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

		if (_fuseComponents and _modelComp.size() > 1)
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

		this->simplify(500, false);

		this->writeBinary(binaryFile);
	}

	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->buildTriangleMeshTopology();
#if !DATASET_GENERATION
		modelComponent->buildPointCloudTopology();
		modelComponent->buildWireframeTopology();
#endif
	}
	this->setVAOData();

	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->releaseMemory(false, false);
		modelComponent->updateSSBO();
	}

	return true;
}

void CADModel::modifyVertices(const mat4& mMatrix)
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

void CADModel::reload()
{
	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->buildTriangleMeshTopology();
		modelComponent->buildPointCloudTopology();
		modelComponent->buildWireframeTopology();

		VAO* vao = modelComponent->_vao;
		if (vao)
		{
			vao->setIBOData(RendEnum::IBO_POINT_CLOUD, modelComponent->_pointCloud);
			vao->setIBOData(RendEnum::IBO_WIREFRAME, modelComponent->_wireframe);
			vao->setIBOData(RendEnum::IBO_TRIANGLE_MESH, modelComponent->_triangleMesh);
		}
	}

	for (ModelComponent* modelComponent : _modelComp)
		modelComponent->releaseMemory(false, false);
}

void CADModel::removeNonManifoldVertices()
{
	for (ModelComponent* modelComponent : _modelComp)
	{
		std::unordered_map<int, std::unordered_map<int, int>> includedEdges;				// Already included edges

		auto isEdgeIncluded = [&](int index1, int index2) -> std::unordered_map<int, std::unordered_map<int, int>>::iterator
			{
				std::unordered_map<int, std::unordered_map<int, int>>::iterator it;

				if ((it = includedEdges.find(index1)) != includedEdges.end())			// p2, p1 and p1, p2 are considered to be the same edge
				{
					if (it->second.find(index2) != it->second.end())					// Already included
					{
						it->second[index2]++;
						return it;
					}
				}

				if ((it = includedEdges.find(index2)) != includedEdges.end())
				{
					if (it->second.find(index1) != it->second.end())					// Already included
					{
						it->second[index1]++;
						return it;
					}
				}

				includedEdges[index1][index2] = 1;

				return includedEdges.end();
			};

		for (unsigned int i = 0; i < modelComponent->_topology.size(); i += 3)
		{
			for (int j = 0; j < 3; ++j)
			{
				isEdgeIncluded(modelComponent->_topology[i]._vertices[j], modelComponent->_topology[i]._vertices[(j + 1) % 3]);
			}
		}

		for (const auto& edge : includedEdges)
		{
			for (const auto& edge2 : edge.second)
			{
				if (edge2.second > 2)
				{
					std::cout << "Edge " << edge.first << ", " << edge2.first << " is included " << edge2.second << " times" << std::endl;
				}
			}
		}
	}
}

PointCloud3D* CADModel::sample(unsigned maxSamples, int randomFunction)
{
	PointCloud3D* pointCloud = nullptr;

	if (_modelComp.size())
	{
		Model3D::ModelComponent* modelComp = _modelComp[0];
		std::vector<float> noiseBuffer;
		fracturer::Seeder::getFloatNoise(maxSamples, maxSamples * 2, randomFunction, noiseBuffer);

		// Max. triangle area
		float maxArea = FLT_MIN;
		Triangle3D triangle;
		for (const Model3D::FaceGPUData& face : modelComp->_topology)
		{
			triangle = Triangle3D(modelComp->_geometry[face._vertices.x]._position, modelComp->_geometry[face._vertices.y]._position, modelComp->_geometry[face._vertices.z]._position);
			maxArea = std::max(maxArea, triangle.area());
		}

		ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::SAMPLER_SHADER);

		unsigned numPoints = 0, noiseBufferSize = 0;
		unsigned maxPoints = maxSamples * modelComp->_topology.size();

		const GLuint vertexSSBO = ComputeShader::setReadBuffer(modelComp->_geometry, GL_STATIC_DRAW);
		const GLuint faceSSBO = ComputeShader::setReadBuffer(modelComp->_topology, GL_STATIC_DRAW);
		const GLuint pointCloudSSBO = ComputeShader::setWriteBuffer(vec4(), maxPoints, GL_DYNAMIC_DRAW);
		const GLuint countingSSBO = ComputeShader::setReadBuffer(&numPoints, 1, GL_DYNAMIC_DRAW);
		const GLuint noiseSSBO = ComputeShader::setReadBuffer(noiseBuffer, GL_STATIC_DRAW);

		shader->use();
		shader->bindBuffers(std::vector<GLuint> { vertexSSBO, faceSSBO, pointCloudSSBO, countingSSBO, noiseSSBO });
		shader->setUniform("maxArea", maxArea);
		//shader->setUniform("noiseBufferSize", static_cast<unsigned>(noiseBuffer.size()));
		shader->setUniform("numSamples", maxSamples);
		shader->setUniform("numTriangles", static_cast<unsigned>(modelComp->_topology.size()));
		if (modelComp->_material) modelComp->_material->applyTexture(shader, Texture::KAD_TEXTURE);
		shader->execute(ComputeShader::getNumGroups(maxSamples * modelComp->_topology.size()), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		numPoints = *ComputeShader::readData(countingSSBO, unsigned());
		vec4* pointCloudData = ComputeShader::readData(pointCloudSSBO, vec4());

		pointCloud = new PointCloud3D;
		pointCloud->push_back(pointCloudData, numPoints);

		GLuint buffers[] = { vertexSSBO, faceSSBO, pointCloudSSBO, countingSSBO, noiseSSBO };
		glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
	}

	return pointCloud;
}

bool CADModel::save(const std::string& filename, unsigned numFaces)
{
	aiScene scene;
	scene.mRootNode = new aiNode();

	scene.mMaterials = new aiMaterial * [1];
	scene.mMaterials[0] = nullptr;
	scene.mNumMaterials = 1;

	scene.mMaterials[0] = new aiMaterial();

	scene.mMeshes = new aiMesh * [1];
	scene.mMeshes[0] = nullptr;
	scene.mNumMeshes = 1;

	scene.mMeshes[0] = new aiMesh();
	scene.mMeshes[0]->mMaterialIndex = 0;

	scene.mRootNode->mMeshes = new unsigned int[1];
	scene.mRootNode->mMeshes[0] = 0;
	scene.mRootNode->mNumMeshes = 1;

	auto pMesh = scene.mMeshes[0];

	// Retrieving info from the model
	glm::uint geometrySize = 0;
	std::vector<glm::vec3> vertices;
	std::vector<ivec3> faces;

	for (ModelComponent* modelComponent : _modelComp)
	{
		for (VertexGPUData& vertex : modelComponent->_geometry)
			vertices.push_back(vertex._position);

		for (FaceGPUData& face : modelComponent->_topology)
			faces.push_back(ivec3(face._vertices.x + geometrySize, face._vertices.y + geometrySize, face._vertices.z + geometrySize));

		geometrySize += modelComponent->_geometry.size();
	}

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

	bool saveSuccess = _assimpExporter.Export(&scene, "stl", filename) == AI_SUCCESS;

	//if (numFaces > 0 && numFaces < this->getNumFaces())
	//{
	//	// Read with CGAL
	//	Mesh mesh;
	//	CGAL::Polygon_mesh_processing::IO::read_polygon_mesh(filename, mesh);

	//	CGAL::Surface_mesh_simplification::Face_count_stop_predicate<Mesh> stop(numFaces);
	//	CGAL::Surface_mesh_simplification::edge_collapse(mesh, stop);

	//	// Write with CGAL
	//	CGAL::IO::write_polygon_mesh(filename, mesh);
	//}

	return saveSuccess;
}

void CADModel::simplify(unsigned numFaces, bool cgal)
{
	for (Model3D::ModelComponent* modelComponent : _modelComp)
	{
		if (modelComponent->_topology.size() > numFaces)
		{
			if (cgal)
			{
				//if (modelComponent->_topology.size() > 10000)
				//	this->simplify(10000, false);

				this->save("Temp/temp.stl");

				Mesh mesh;
				CGAL::Polygon_mesh_processing::IO::read_polygon_mesh("Temp/temp.stl", mesh);

				CGAL::Surface_mesh_simplification::Face_count_stop_predicate<Mesh> stop(numFaces);
				CGAL::Surface_mesh_simplification::edge_collapse(mesh, stop);

				//Mesh* mesh = CGALInterface::translateMesh(modelComponent);

				//CGAL::Surface_mesh_simplification::Face_count_stop_predicate<Mesh> stop(numFaces);
				//CGAL::Surface_mesh_simplification::edge_collapse(*mesh, stop);

				//delete mesh;

				CGALInterface::translateMesh(&mesh, modelComponent);
			}
			else
			{
				Simplify::vertices.clear();
				Simplify::triangles.clear();

				for (const Model3D::VertexGPUData& vertex : modelComponent->_geometry)
					Simplify::vertices.push_back(Simplify::Vertex{ vec3f(vertex._position.x, vertex._position.y, vertex._position.z) });

				for (const Model3D::FaceGPUData& face : modelComponent->_topology)
				{
					Simplify::triangles.push_back(Simplify::Triangle());
					for (int i = 0; i < 3; ++i)
						Simplify::triangles[Simplify::triangles.size() - 1].v[i] = face._vertices[i];
				}

				//Simplify::simplify_mesh(numFaces, 5.0);

				modelComponent->_geometry.clear();
				modelComponent->_topology.clear();

				for (const Simplify::Vertex& vertex : Simplify::vertices)
					modelComponent->_geometry.push_back(Model3D::VertexGPUData{ vec3(vertex.p.x, vertex.p.y, vertex.p.z) });

				for (const Simplify::Triangle& triangle : Simplify::triangles)
					modelComponent->_topology.push_back(Model3D::FaceGPUData{ uvec3(triangle.v[0], triangle.v[1], triangle.v[2]) });
			}
		}

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

/// [Protected methods]

void CADModel::computeMeshData(ModelComponent* modelComp, bool computeNormals)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::MODEL_MESH_GENERATION);
	const int arraySize = modelComp->_topology.size();
	const int numGroups = ComputeShader::getNumGroups(arraySize);

	GLuint modelBufferID, meshBufferID;
	modelBufferID = ComputeShader::setReadBuffer(modelComp->_geometry);
	meshBufferID = ComputeShader::setReadBuffer(modelComp->_topology);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID, meshBufferID });
	shader->use();
	shader->setUniform("size", arraySize);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	FaceGPUData* faceData = shader->readData(meshBufferID, FaceGPUData());
	modelComp->_topology = std::move(std::vector<FaceGPUData>(faceData, faceData + arraySize));

	if (computeNormals)
	{
		//#pragma omp parallel for
		std::vector<unsigned> faceCount(modelComp->_geometry.size(), 0);
		for (int faceIdx = 0; faceIdx < modelComp->_topology.size(); ++faceIdx)
		{
			for (int i = 0; i < 3; ++i)
			{
				modelComp->_geometry[modelComp->_topology[faceIdx]._vertices[i]]._normal += modelComp->_topology[faceIdx]._normal;
				++faceCount[modelComp->_topology[faceIdx]._vertices[i]];
			}
		}

#pragma omp parallel for
		for (int vertexIdx = 0; vertexIdx < modelComp->_geometry.size(); ++vertexIdx)
		{
			modelComp->_geometry[vertexIdx]._normal /= faceCount[vertexIdx];
			modelComp->_geometry[vertexIdx]._normal = glm::normalize(modelComp->_geometry[vertexIdx]._normal);
		}
	}

	glDeleteBuffers(1, &modelBufferID);
	glDeleteBuffers(1, &meshBufferID);
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
		fin.read((char*)&component->_name[0], length);

		fin.read((char*)&component->_aabb, sizeof(AABB));

		// Recover material description
		fin.read((char*)&length, sizeof(size_t));
		component->_materialDescription._rootFolder.resize(length);
		fin.read((char*)&component->_materialDescription._rootFolder[0], length);

		fin.read((char*)&length, sizeof(size_t));
		component->_materialDescription._name.resize(length);
		fin.read((char*)&component->_materialDescription._name[0], length);

		for (int textureLayer = 0; textureLayer < Texture::NUM_TEXTURE_TYPES; textureLayer += 1)
		{
			fin.read((char*)&length, sizeof(size_t));
			component->_materialDescription._textureImage[textureLayer].resize(length);
			if (length) fin.read((char*)&component->_materialDescription._textureImage[textureLayer][0], length);
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
		fout.write((char*)&component->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

		const size_t numTriangles = component->_topology.size();
		fout.write((char*)&numTriangles, sizeof(size_t));
		fout.write((char*)&component->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

		numIndices = component->_triangleMesh.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		fout.write((char*)&component->_triangleMesh[0], numIndices * sizeof(GLuint));

		numIndices = component->_pointCloud.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		fout.write((char*)&component->_pointCloud[0], numIndices * sizeof(GLuint));

		numIndices = component->_wireframe.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		fout.write((char*)&component->_wireframe[0], numIndices * sizeof(GLuint));

		size_t nameLength = component->_name.size();
		fout.write((char*)&nameLength, sizeof(size_t));
		fout.write((char*)&component->_name[0], nameLength);

		fout.write((char*)(&component->_aabb), sizeof(AABB));

		// Write material description
		size_t rootFolderLength = component->_materialDescription._rootFolder.size(), length;
		fout.write((char*)&rootFolderLength, sizeof(size_t));
		fout.write((char*)&component->_materialDescription._rootFolder[0], rootFolderLength);

		nameLength = component->_materialDescription._name.size();
		fout.write((char*)&nameLength, sizeof(size_t));
		fout.write((char*)&component->_materialDescription._name[0], nameLength);

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