#include "stdafx.h"
#include "AABBSet.h"

#include "Graphics/Core/OpenGLUtilities.h"

// [Public methods]

AABBSet::AABBSet(std::vector<AABB>& aabbs) : Model3D(mat4(1.0f), 1), _aabb(aabbs)
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

		// Multi-instancing VBOs
		std::vector<vec3> offset, scale;

		for (AABB& aabb: _aabb)
		{
			offset.push_back(aabb.center());
			scale.push_back(aabb.extent() * 2.0f);
		}

		vao->defineMultiInstancingVBO(RendEnum::VBO_OFFSET, vec3(), .0f, GL_FLOAT);
		vao->defineMultiInstancingVBO(RendEnum::VBO_SCALE, vec3(), .0f, GL_FLOAT);
		vao->defineMultiInstancingVBO(RendEnum::VBO_INDEX, float(), .0f, GL_FLOAT);

		vao->setVBOData(RendEnum::VBO_OFFSET, offset);
		vao->setVBOData(RendEnum::VBO_SCALE, scale);

		_modelComp[0]->_vao = vao;	
		_loaded = true;
	}

	return true;
}

void AABBSet::setColorIndex(std::vector<uint16_t>& colorBuffer)
{
	std::vector<float> colorIndex(colorBuffer.size());

	for (int idx = 0; idx < colorBuffer.size(); ++idx)
	{
		colorIndex[idx] = colorBuffer[idx];
	}
	
	_modelComp[0]->_vao->setVBOData(RendEnum::VBO_INDEX, colorIndex);
}

// [Protected methods]

void AABBSet::renderTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;
	Material* material = modelComp->_material;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		if (material) material->applyMaterial(shader);

		vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, 64, _aabb.size());
	}
}
