#include "stdafx.h"
#include "DrawLines.h"

#include "Graphics/Core/VAO.h"

// [Public methods]

DrawLines::DrawLines(const std::vector<vec3>& points): Model3D()
{
	for (int pointIdx = 0; pointIdx < points.size(); ++pointIdx)
	{
		_modelComp[0]->_geometry.push_back(Model3D::VertexGPUData{ points[pointIdx] });
	}

	for (int pointIdx = 0; pointIdx < points.size() - 1; ++pointIdx)
	{
		_modelComp[0]->_wireframe.push_back(pointIdx);
		_modelComp[0]->_wireframe.push_back(pointIdx + 1);
		_modelComp[0]->_wireframe.push_back(RESTART_PRIMITIVE_INDEX);
	}

	Model3D::setVAOData(true);
}

DrawLines::~DrawLines()
{
}

bool DrawLines::load()
{
	return false;
}
