#include "stdafx.h"
#include "SSAOScene.h"

#include "Geometry/3D/Intersections3D.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/RenderingShader.h"
#include "Graphics/Core/ShaderList.h"
#include "Interface/Window.h"

/// [Public methods]

SSAOScene::SSAOScene(): Scene()
{
}

SSAOScene::~SSAOScene()
{
	for (Group3D::StaticGPUData* staticData : _sceneGPUData) delete staticData;
}

void SSAOScene::load()
{	
	Scene::load();
}

void SSAOScene::render(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera();

	this->drawAsTriangles4Shadows(mModel, rendParams);

	if (rendParams->_ambientOcclusion && this->needToApplyAmbientOcclusion(rendParams))
	{
		_ssaoFBO->bindMultisamplingFBO(rendParams);
		this->renderScene(mModel, rendParams);
		_ssaoFBO->writeGBuffer(0);

		_ssaoFBO->bindGBufferFBO(1);
		if (rendParams->_visualizationMode >= CGAppEnum::VIS_TRIANGLES) this->drawAsTriangles4Position(mModel, rendParams);

		_ssaoFBO->bindGBufferFBO(2);
		if (rendParams->_visualizationMode >= CGAppEnum::VIS_TRIANGLES) this->drawAsTriangles4Normal(mModel, rendParams);

		_ssaoFBO->bindSSAOFBO();
		this->drawSSAOScene();

		this->bindDefaultFramebuffer(rendParams);
		this->composeScene(rendParams);
	}
	else
	{
		this->bindDefaultFramebuffer(rendParams);

		this->renderScene(mModel, rendParams);
	}
}

/// [Protected methods]

void SSAOScene::loadModels()
{
	for (Group3D* group: _sceneGroup)
	{
		group->load();
		group->registerScene();	
	}
}

bool SSAOScene::needToApplyAmbientOcclusion(RenderingParameters* rendParams)
{
	return true/*!((rendParams->_showLidarPointCloud && _pointCloud->getNumPoints()) || rendParams->_visualizationMode == CGAppEnum::VIS_POINTS || rendParams->_visualizationMode == CGAppEnum::VIS_LINES)*/;
}

void SSAOScene::renderOtherStructures(const mat4& mModel, RenderingParameters* rendParams)
{
}

void SSAOScene::renderUniformPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = mModel;
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	shader->use();
	shader->applyActiveSubroutines();

	for (Group3D* group: _sceneGroup) group->drawAsPoints(shader, RendEnum::POINT_CLOUD_SHADER, matrix);
}

void SSAOScene::renderScene(const mat4& mModel, RenderingParameters* rendParams)
{
	int visualizationMode = rendParams->_visualizationMode;

	glPolygonOffset(1.0f, 1.0f);

	switch (visualizationMode)
	{
	case CGAppEnum::VIS_POINTS:
		this->renderPointCloud(mModel, rendParams);
		break;
	case CGAppEnum::VIS_LINES:
		this->renderWireframe(mModel, rendParams);
		break;
	case CGAppEnum::VIS_TRIANGLES:
		this->renderTriangleMesh(mModel, rendParams);
		break;
	case CGAppEnum::VIS_ALL_TOGETHER:
		this->renderTriangleMesh(mModel, rendParams);
		this->renderWireframe(mModel, rendParams);
		this->renderPointCloud(mModel, rendParams);
		break;
	}

	this->renderOtherStructures(mModel, rendParams);

	glPolygonOffset(0.0f, 0.0f);
}

void SSAOScene::renderPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	this->drawAsPoints(mModel, rendParams);
}

void SSAOScene::renderWireframe(const mat4& mModel, RenderingParameters* rendParams)
{
	this->drawAsLines(mModel, rendParams);
}

void SSAOScene::renderTriangleMesh(const mat4& mModel, RenderingParameters* rendParams)
{
	SSAOScene::drawAsTriangles(mModel, rendParams);
}

// ------------------------- Draw scene ------------------------------

void SSAOScene::drawSceneAsTriangles(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	Scene::drawSceneAsTriangles(shader, shaderType, matrix, rendParams);
}

void SSAOScene::drawSceneAsTriangles4Normal(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	Scene::drawSceneAsTriangles4Normal(shader, shaderType, matrix, rendParams);
}

void SSAOScene::drawSceneAsTriangles4Position(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	Scene::drawSceneAsTriangles4Position(shader, shaderType, matrix, rendParams);
}
