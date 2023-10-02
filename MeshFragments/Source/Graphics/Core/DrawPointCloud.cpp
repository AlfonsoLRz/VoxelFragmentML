#include "stdafx.h"
#include "DrawPointCloud.h"

#include "Graphics/Core/VAO.h"

// [Public methods]

DrawPointCloud::DrawPointCloud(PointCloud3D* pointCloud): Model3D()
{
	std::vector<vec4>* points = pointCloud->getPoints();
	std::vector<float>* colors = pointCloud->getColors();

	for (int pointIdx = 0; pointIdx < points->size(); ++pointIdx)
	{
		_modelComp[0]->_geometry.push_back(Model3D::VertexGPUData{ points->at(pointIdx) });
		_modelComp[0]->_pointCloud.push_back(pointIdx);
	}

	Model3D::setVAOData(true);

	if (!colors->empty())
	{
		_modelComp[0]->_vao->defineVBO(RendEnum::VBOTypes::VBO_COLOR, .0f, GL_FLOAT);
		_modelComp[0]->_vao->setVBOData(RendEnum::VBOTypes::VBO_COLOR, colors->data(), colors->size());
	}

	_modelComp[0]->_vao->defineVBO(RendEnum::VBOTypes::VBO_CLUSTER_ID, .0f, GL_FLOAT);

	_modelComp[0]->releaseMemory();
}

DrawPointCloud::~DrawPointCloud()
{
}

bool DrawPointCloud::load(const mat4& modelMatrix)
{
	return false;
}
