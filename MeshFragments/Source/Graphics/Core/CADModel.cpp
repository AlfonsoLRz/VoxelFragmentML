#include "stdafx.h"
#include "CADModel.h"

#include <filesystem>
#include "Graphics/Application/MaterialList.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/VAO.h"
#include "Utilities/FileManagement.h"
#include "Utilities/ChronoUtilities.h"

// Initialization of static attributes
std::unordered_map<std::string, std::unique_ptr<Material>> CADModel::_cadMaterials;
std::unordered_map<std::string, std::unique_ptr<Texture>> CADModel::_cadTextures;

const std::string CADModel::BINARY_EXTENSION = ".bin";
const std::string CADModel::OBJ_EXTENSION = ".obj";

/// [Public methods]

CADModel::CADModel(const std::string& filename, const std::string& textureFolder, const bool useBinary) : 
	Model3D(mat4(1.0f), 0)
{
	_filename = filename;
	_textureFolder = textureFolder;
	_useBinary = useBinary;
}

CADModel::CADModel(const std::vector<Triangle3D>& triangles, const mat4& modelMatrix) :
	Model3D(modelMatrix, 1), _useBinary(false)
{
	for (const Triangle3D& triangle : triangles)
	{
		vec3 normal = triangle.normal();
		for (int i = 0; i < 3; ++i)
		{
			_modelComp[0]->_geometry.push_back(Model3D::VertexGPUData{ triangle.getPoint(i), .0f, normal });
			_modelComp[0]->_triangleMesh.push_back(_modelComp[0]->_geometry.size() - 1);
		}
	}

	Model3D::setVAOData();
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
		}

		if (!binaryExists && success)
		{
			this->writeToBinary();
		}

		this->subdivide(0.0001f);
		for (ModelComponent* modelComponent : _modelComp)
		{
			modelComponent->buildTriangleMeshTopology();
			modelComponent->buildPointCloudTopology();
			modelComponent->buildWireframeTopology();
		}
		this->setVAOData();

		return _loaded = true;
	}

	return false;
}

void CADModel::subdivide(float maxArea)
{
	bool applyChanges = false;

	#pragma omp parallel for
	for (int modelCompIdx = 0; modelCompIdx < _modelComp.size(); ++modelCompIdx)
	{
		bool applyChangesComp = false;
		Model3D::ModelComponent* modelComp = _modelComp[modelCompIdx];
		unsigned numFaces = modelComp->_topology.size();

		for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
		{
			Triangle3D triangle(modelComp->_geometry[modelComp->_topology[faceIdx]._vertices.x]._position,
				modelComp->_geometry[modelComp->_topology[faceIdx]._vertices.y]._position,
				modelComp->_geometry[modelComp->_topology[faceIdx]._vertices.z]._position);

			if (triangle.area() > maxArea)
			{
				// Subdivide
				triangle.subdivide(modelComp->_geometry, modelComp->_topology, modelComp->_topology[faceIdx], maxArea);
				applyChangesComp = true;
				modelComp->_topology.erase(modelComp->_topology.begin() + faceIdx);
				--faceIdx;
			}
		}

		#pragma omp critical
		applyChanges &= applyChangesComp;
	}
}

/// [Protected methods]

void CADModel::computeMeshData(ModelComponent* modelComp)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::MODEL_MESH_GENERATION);
	const int arraySize = modelComp->_topology.size();
	const int numGroups = ComputeShader::getNumGroups(arraySize);

	GLuint modelBufferID, meshBufferID, outBufferID;
	modelBufferID = ComputeShader::setReadBuffer(modelComp->_geometry);
	meshBufferID = ComputeShader::setReadBuffer(modelComp->_topology);
	outBufferID = ComputeShader::setWriteBuffer(GLuint(), arraySize * 4);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID, meshBufferID, outBufferID });
	shader->use();
	shader->setUniform("size", arraySize);
	shader->setUniform("restartPrimitiveIndex", Model3D::RESTART_PRIMITIVE_INDEX);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	FaceGPUData* faceData = shader->readData(meshBufferID, FaceGPUData());
	GLuint* rawMeshData = shader->readData(outBufferID, GLuint());
	modelComp->_topology = std::move(std::vector<FaceGPUData>(faceData, faceData + arraySize));
	modelComp->_triangleMesh = std::move(std::vector<GLuint>(rawMeshData, rawMeshData + arraySize * 4));

	glDeleteBuffers(1, &modelBufferID);
	glDeleteBuffers(1, &meshBufferID);
	glDeleteBuffers(1, &outBufferID);
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

void CADModel::generateGeometryTopology(Model3D::ModelComponent* modelComp, const mat4& modelMatrix)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::MODEL_APPLY_MODEL_MATRIX);
	const int arraySize = modelComp->_geometry.size();
	const int numGroups = ComputeShader::getNumGroups(arraySize);

	GLuint modelBufferID;
	modelBufferID = ComputeShader::setReadBuffer(modelComp->_geometry);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID});
	shader->use();
	shader->setUniform("mModel", modelMatrix * _modelMatrix);
	shader->setUniform("size", arraySize);
	if (modelComp->_material) modelComp->_material->applyMaterial4ComputeShader(shader);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	VertexGPUData* data = shader->readData(modelBufferID, VertexGPUData());
	modelComp->_geometry = std::move(std::vector<VertexGPUData>(data, data + arraySize));

	this->computeTangents(modelComp);
	this->computeMeshData(modelComp);

	glDeleteBuffers(1, &modelBufferID);

	// Wireframe & point cloud are derived from previous operations
	//modelComp->buildPointCloudTopology();
	//modelComp->buildWireframeTopology();
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
				this->generateGeometryTopology(_modelComp[modelCompIdx], modelMatrix);
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