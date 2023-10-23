#pragma once

#include "Geometry/3D/PointCloud3D.h"
#include "Graphics/Core/Model3D.h"

class DrawPointCloud: public Model3D
{
public:
	DrawPointCloud(PointCloud3D* pointCloud);
	virtual ~DrawPointCloud();

	virtual bool load();
};

