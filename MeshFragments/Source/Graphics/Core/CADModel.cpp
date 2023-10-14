#include "stdafx.h"
#include "CADModel.h"

#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Face_count_stop_predicate.h>
#include "CGALInterface.h"
#include <filesystem>
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
const std::string CADModel::OBJ_EXTENSION = ".obj";

/// [Public methods]

CADModel::CADModel(const std::string& filename, const std::string& textureFolder, const bool useBinary, const bool fuseComponents, const bool mergeVertices) : 
	Model3D(mat4(1.0f), 0)
{
	_filename = filename;
	_fuseComponents = fuseComponents;
	_fuseVertices = mergeVertices;
	_textureFolder = textureFolder;
	_useBinary = useBinary;
}

CADModel::CADModel(const std::vector<Triangle3D>& triangles, bool releaseMemory, const mat4& modelMatrix) :
	Model3D(modelMatrix, 1), _useBinary(false)
{
	for (const Triangle3D& triangle : triangles)
	{
		vec3 normal = triangle.normal();
		for (int i = 0; i < 3; ++i)
		{
			_modelComp[0]->_geometry.push_back(Model3D::VertexGPUData{ triangle.getPoint(i), .0f, normal });
			//_modelComp[0]->_triangleMesh.push_back(_modelComp[0]->_geometry.size() - 1);
		}
		_modelComp[0]->_topology.push_back(Model3D::FaceGPUData{ uvec3(_modelComp[0]->_geometry.size() - 3, _modelComp[0]->_geometry.size() - 2, _modelComp[0]->_geometry.size() - 1) });
	}

	//std::vector<int> mapping (_modelComp[0]->_geometry.size());
	//std::iota(mapping.begin(), mapping.end(), 0);

	//this->fuseVertices(mapping);
	//this->remapVertices(_modelComp[0], mapping);
	//this->simplify(1000);

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

CADModel::CADModel(Model3D::VertexGPUData* vertices, unsigned numVertices, Model3D::FaceGPUData* faces, unsigned numFaces, bool releaseMemory, const mat4& modelMatrix): 
	Model3D(modelMatrix, 1), _useBinary(false)
{
	ModelComponent* modelComponent = _modelComp[0];

	modelComponent->_geometry.insert(modelComponent->_geometry.end(), vertices, vertices + numVertices);
	modelComponent->_topology.insert(modelComponent->_topology.end(), faces, faces + numFaces);

	this->simplify(1000);
	this->computeMeshData(modelComponent, true);

	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->buildTriangleMeshTopology();
		modelComponent->buildWireframeTopology();
	}

	Model3D::setVAOData();

	for (ModelComponent* modelComponent : _modelComp)
		modelComponent->releaseMemory(releaseMemory, releaseMemory);
}

CADModel::~CADModel()
{
}

bool CADModel::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{
		bool success = false, binaryExists = false;

		if (_useBinary && (binaryExists = std::filesystem::exists(_filename + BINARY_EXTENSION)))
		{
			success = this->loadModelFromBinaryFile();
		}
			
		if (!success)
		{
			success = this->loadModelFromOBJ(modelMatrix);
			if (success and _fuseComponents and _modelComp.size() > 1)
				this->fuseComponents();

			this->subdivide(0.0001f);

			if (_fuseVertices)
			{
				for (Model3D::ModelComponent* modelComp : _modelComp)
				{
					std::vector<int> mapping (_modelComp[0]->_geometry.size());
					std::iota(mapping.begin(), mapping.end(), 0);

					this->fuseVertices(mapping);
					this->remapVertices(_modelComp[0], mapping);
				}
			}
		}

		if (!binaryExists && success)
		{
			this->writeToBinary();
		}

		for (ModelComponent* modelComponent : _modelComp)
		{
			modelComponent->buildTriangleMeshTopology();
			//modelComponent->buildPointCloudTopology();
			//modelComponent->buildWireframeTopology();
		}
		this->setVAOData();
		
		for (ModelComponent* modelComponent : _modelComp)
		{
			modelComponent->releaseMemory(false, false);
			modelComponent->updateSSBO();
		}

		return _loaded = true;
	}

	return false;
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

		const GLuint vertexSSBO		= ComputeShader::setReadBuffer(modelComp->_geometry, GL_STATIC_DRAW);
		const GLuint faceSSBO		= ComputeShader::setReadBuffer(modelComp->_topology, GL_STATIC_DRAW);
		const GLuint pointCloudSSBO = ComputeShader::setWriteBuffer(vec4(), maxPoints, GL_DYNAMIC_DRAW);
		const GLuint countingSSBO	= ComputeShader::setReadBuffer(&numPoints, 1, GL_DYNAMIC_DRAW);
		const GLuint noiseSSBO		= ComputeShader::setReadBuffer(noiseBuffer, GL_STATIC_DRAW);

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

void CADModel::simplify(unsigned numFaces)
{
	for (Model3D::ModelComponent* modelComponent : _modelComp)
	{
		if (modelComponent->_topology.size() > numFaces)
		{
#if CGAL_SIMPLIFICATION
			Mesh* mesh = CGALInterface::translateMesh(modelComponent);

			CGAL::Surface_mesh_simplification::Face_count_stop_predicate<Mesh> stop(numFaces);
			CGAL::Surface_mesh_simplification::edge_collapse(*mesh, stop);

			CGALInterface::translateMesh(mesh, modelComponent);

			delete mesh;
#else
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

			Simplify::simplify_mesh(numFaces);

			modelComponent->_geometry.clear();
			modelComponent->_topology.clear();

			for (const Simplify::Vertex& vertex : Simplify::vertices)
				modelComponent->_geometry.push_back(Model3D::VertexGPUData{vec3(vertex.p.x, vertex.p.y, vertex.p.z)});

			for (const Simplify::Triangle& triangle : Simplify::triangles)
				modelComponent->_topology.push_back(Model3D::FaceGPUData{uvec3(triangle.v[0], triangle.v[1], triangle.v[2])});
#endif
		}
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
	Model3D::ModelComponentDescription* modelDescription = &modelComp->_modelDescription;
	std::string name = std::string(modelDescription->_materialName);
	std::string mapKd = std::string(modelDescription->_mapKd);
	std::string mapKs = std::string(modelDescription->_mapKs);

	if (!name.empty() && name != nullMaterialName)
	{
		auto itMaterial = _cadMaterials.find(name);

		if (itMaterial == _cadMaterials.end())
		{
			material = new Material();
			Texture* kad, * ks;

			if (!mapKd.empty())
			{
				kad = new Texture(_textureFolder + mapKd);
			}
			else
			{
				kad = new Texture(vec4(modelDescription->_kd, 1.0f));
			}

			if (!mapKs.empty())
			{
				ks = new Texture(_textureFolder + mapKs);
			}
			else
			{
				ks = new Texture(vec4(modelDescription->_ks, 1.0f));
				material->setShininess(modelDescription->_ns);
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

void CADModel::createModelComponent(objl::Mesh* mesh)
{
	const GLuint maxVertices	= ComputeShader::getMaxSSBOSize(sizeof(VertexGPUData)) / 3;
	const GLuint maxFaces		= ComputeShader::getMaxSSBOSize(sizeof(FaceGPUData));
	VertexGPUData vertexData;

	// Defined number of splits of the mesh
	const unsigned firstModelComp	= _modelComp.size();
	unsigned leftVertices			= mesh->Vertices.size(), currentNumVertices = 0, thresholdVertex = 0;
	unsigned startVertexIdx			= 0, startFaceIdx = UINT_MAX, k = 0, startingK = 0;
	uvec3 vertices;

	while (leftVertices > 0)
	{
		ModelComponent* modelComp	= new ModelComponent(this);
		currentNumVertices			= std::min(leftVertices, maxVertices);
		thresholdVertex				= startVertexIdx + currentNumVertices;

		for (int j = startVertexIdx; j < thresholdVertex && j < mesh->Vertices.size(); j++)
		{
			vertexData._position	= vec3(mesh->Vertices[j].Position.X, mesh->Vertices[j].Position.Y, mesh->Vertices[j].Position.Z);
			vertexData._normal		= vec3(mesh->Vertices[j].Normal.X, mesh->Vertices[j].Normal.Y, mesh->Vertices[j].Normal.Z);
			vertexData._textCoord	= vec2(mesh->Vertices[j].TextureCoordinate.X, mesh->Vertices[j].TextureCoordinate.Y);

			modelComp->_geometry.push_back(vertexData);
		}

		startVertexIdx = thresholdVertex;

		for (; k < mesh->Indices.size(); k += 3)
		{
			vertices = uvec3(mesh->Indices[k], mesh->Indices[k + 1], mesh->Indices[k + 2]);

			if (vertices.x < thresholdVertex && vertices.y < thresholdVertex && vertices.z < thresholdVertex)
			{
				modelComp->_topology.push_back(Model3D::FaceGPUData());
				modelComp->_topology[(k - startingK) / 3]._vertices = uvec3(mesh->Indices[k], mesh->Indices[k + 1], mesh->Indices[k + 2]) - uvec3(startVertexIdx - currentNumVertices);
				modelComp->_topology[(k - startingK) / 3]._modelCompID = modelComp->_id;
			}
			else
			{
				if (vertices.x < thresholdVertex)
				{
					startVertexIdx	= std::min(vertices.x, startVertexIdx);
					startFaceIdx	= std::min(k, startFaceIdx);
				}
				else if (vertices.y < thresholdVertex)
				{
					startVertexIdx	= std::min(vertices.y, startVertexIdx);
					startFaceIdx	= std::min(k, startFaceIdx);
				}
				else if (vertices.z < thresholdVertex)
				{
					startVertexIdx	= std::min(vertices.z, startVertexIdx);
					startFaceIdx	= std::min(k, startFaceIdx);
				}
				else
				{
					break;			// All indices are above the threshold
				}
			}
		}

		k = startingK = std::min(k, startFaceIdx);
		startFaceIdx = UINT_MAX;

		if (!modelComp->_material)
		{
			modelComp->_modelDescription	= Model3D::ModelComponentDescription(mesh);
			modelComp->_material			= this->createMaterial(modelComp);
			modelComp->setName(modelComp->_modelDescription._modelName);
		}

		_modelComp.push_back(modelComp);
		leftVertices = mesh->Vertices.size() - startVertexIdx;
	}
}

void CADModel::fuseComponents()
{
	Model3D::ModelComponent* newModelComponent = new Model3D::ModelComponent(this);
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

bool CADModel::loadModelFromBinaryFile()
{
	bool success;

	if (success = this->readBinary(_filename +  BINARY_EXTENSION, _modelComp))
	{
		for (ModelComponent* modelComp : _modelComp)
		{
			modelComp->_material = this->createMaterial(modelComp);
			modelComp->setName(modelComp->_modelDescription._modelName);
		}
	}

	return success;
}

bool CADModel::loadModelFromOBJ(const mat4& modelMatrix)
{
	objl::Loader loader;
	bool success = loader.LoadFile(_filename + OBJ_EXTENSION);

	if (success)
	{
		std::string modelName, currentModelName;

		if (loader.LoadedMeshes.size())
		{
			unsigned index = 0;

			while (index < loader.LoadedMeshes.size() && modelName.empty())
			{
				modelName = loader.LoadedMeshes[0].MeshName;
				++index;
			}
		}

		for (int i = 0; i < loader.LoadedMeshes.size(); i++)
		{
			unsigned modelCompIdx = _modelComp.size();
			
			this->createModelComponent(&(loader.LoadedMeshes[i]));
			while (modelCompIdx < _modelComp.size())
			{
				this->computeMeshData(_modelComp[modelCompIdx]);
				++modelCompIdx;
			}
		}
	}

	return success;
}

bool CADModel::readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp)
{
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	if (!fin.is_open())
	{
		return false;
	}

	size_t numModelComps, numVertices, numTriangles, numIndices;

	fin.read((char*)&numModelComps, sizeof(size_t));
	while (_modelComp.size() < numModelComps)
	{
		_modelComp.push_back(new ModelComponent(this));
	}

	for (Model3D::ModelComponent* model : modelComp)
	{
		fin.read((char*)&numVertices, sizeof(size_t));
		model->_geometry.resize(numVertices);
		fin.read((char*)&model->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

		fin.read((char*)&numTriangles, sizeof(size_t));
		model->_topology.resize(numTriangles);
		fin.read((char*)&model->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

		fin.read((char*)&numIndices, sizeof(size_t));
		model->_triangleMesh.resize(numIndices);
		fin.read((char*)&model->_triangleMesh[0], numIndices * sizeof(GLuint));

		fin.read((char*)&numIndices, sizeof(size_t));
		model->_pointCloud.resize(numIndices);
		fin.read((char*)&model->_pointCloud[0], numIndices * sizeof(GLuint));

		fin.read((char*)&numIndices, sizeof(size_t));
		model->_wireframe.resize(numIndices);
		fin.read((char*)&model->_wireframe[0], numIndices * sizeof(GLuint));

		fin.read((char*)&model->_modelDescription, sizeof(Model3D::ModelComponentDescription));
	}

	fin.close();

	return true;
}

void CADModel::remapVertices(Model3D::ModelComponent* modelComponent, std::vector<int>& mapping)
{
	unsigned erasedVertices = 0, startingSize = modelComponent->_geometry.size();
	std::vector<unsigned> newMapping (startingSize);

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

bool CADModel::writeToBinary()
{
	std::ofstream fout(_filename + ".bin", std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		return false;
	}

	size_t numIndices;
	const size_t numModelComps = _modelComp.size();
	fout.write((char*)&numModelComps, sizeof(size_t));

	for (Model3D::ModelComponent* model : _modelComp)
	{
		const size_t numVertices = model->_geometry.size();
		fout.write((char*)&numVertices, sizeof(size_t));
		fout.write((char*)&model->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

		const size_t numTriangles = model->_topology.size();
		fout.write((char*)&numTriangles, sizeof(size_t));
		fout.write((char*)&model->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

		numIndices = model->_triangleMesh.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		fout.write((char*)&model->_triangleMesh[0], numIndices * sizeof(GLuint));

		numIndices = model->_pointCloud.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		fout.write((char*)&model->_pointCloud[0], numIndices * sizeof(GLuint));

		numIndices = model->_wireframe.size();
		fout.write((char*)&numIndices, sizeof(size_t));
		fout.write((char*)&model->_wireframe[0], numIndices * sizeof(GLuint));

		fout.write((char*)&model->_modelDescription, sizeof(Model3D::ModelComponentDescription));
	}

	fout.close();

	return true;
}