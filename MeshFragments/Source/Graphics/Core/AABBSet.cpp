#include "stdafx.h"
#include "AABBSet.h"

#include "DataStructures/RegularGrid.h"
#include "Graphics/Core/OpenGLUtilities.h"

// [Public methods]

AABBSet::AABBSet() : Model3D(mat4(1.0f), 1), _numAABBs(0)
{
}

AABBSet::~AABBSet()
{
}

bool AABBSet::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{
		VAO* vao = Primitives::getCubeVAO();
		_modelComp[0]->_vao = vao;

		vao->defineMultiInstancingVBO(RendEnum::VBO_OFFSET, vec3(), .0f, GL_FLOAT);
		vao->defineMultiInstancingVBO(RendEnum::VBO_SCALE, vec3(), .0f, GL_FLOAT);
		vao->defineMultiInstancingVBO(RendEnum::VBO_INDEX, float(), .0f, GL_FLOAT);
		
		_loaded = true;
	}

	return true;
}

void AABBSet::homogenize()
{
	std::vector<float> colorIndex(_numAABBs);

	for (int idx = 0; idx < _numAABBs; ++idx)
	{
		colorIndex[idx] = VOXEL_FREE;
	}

	_modelComp[0]->_vao->setVBOData(RendEnum::VBO_INDEX, colorIndex);
}

void AABBSet::load(std::vector<AABB>& aabbs)
{
	// Multi-instancing VBOs
	std::vector<vec3> offset, scale;
	VAO* vao = _modelComp[0]->_vao;

	for (AABB& aabb : aabbs)
	{
		offset.push_back(aabb.center());
		scale.push_back(aabb.extent() * 2.0f);
	}

	vao->setVBOData(RendEnum::VBO_OFFSET, offset);
	vao->setVBOData(RendEnum::VBO_SCALE, scale);

	_numAABBs = aabbs.size();
}

void AABBSet::setColorIndex(uint16_t* colorBuffer, unsigned size)
{
	std::vector<float> colorIndex;

	for (int idx = 0; idx < size; ++idx)
	{
		if (colorBuffer[idx] != VOXEL_EMPTY)
		{
			colorIndex.push_back(colorBuffer[idx]);
		}
	}
	
	_modelComp[0]->_vao->setVBOData(RendEnum::VBO_INDEX, colorIndex);
}

// [Protected methods]

void AABBSet::renderTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;
	Material* material = modelComp->_material;

	if (vao && _numAABBs && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		if (material) material->applyMaterial(shader);

		vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, 64, _numAABBs);
	}
}

void AABBSet::renderTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;

	if (vao && _numAABBs && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, 64, _numAABBs);
	}
}