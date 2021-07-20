#pragma once

/**
*	@file AABBSet.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 18/07/2021
*/

#include "Geometry/3D/AABB.h"
#include "Graphics/Core/Model3D.h"

/**
*	@brief Set of bounding boxes to be rendered/processed.
*/
class AABBSet: public Model3D
{
protected:
	std::vector<AABB> _aabb;

protected:
	/**
	*	@brief Renders a component as a set of triangles.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);
	
	/**
	*	@brief Renders a component as a set of lines.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

public:
	/**
	*	@brief Constructor from a set of bounding boxes. 
	*/
	AABBSet(std::vector<AABB>& aabbs);

	/**
	*	@brief Destructor.
	*/
	virtual ~AABBSet();

	/**
	*	@brief Computes the model data and sends it to GPU.
	*	@param modelMatrix Model matrix to be applied while generating geometry.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Setup VAO to integrate colors of each voxel.
	*/
	void setColorIndex(std::vector<uint16_t>& colorBuffer);
};

