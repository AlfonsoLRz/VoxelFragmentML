#version 450

layout (location = 0) in vec4	vPosition;
layout (location = 8) in float	vClusterIndex;
layout (location = 9) in float	vPackedColor;

#include <Assets/Shaders/Compute/Templates/colorHSV.glsl>

uniform mat4	mModelViewProj;
uniform float	pointSize;	
uniform vec3	vColor;

out vec3		color;

// ________ COLOUR SOURCE _________

subroutine vec3 colorType();
subroutine uniform colorType colorUniform;

subroutine(colorType)
vec3 uniformColor()
{
	return vColor;
}

subroutine(colorType)
vec3 textureColor()
{
	return unpackUnorm4x8(uint(vPackedColor)).rgb;
}

subroutine(colorType)
vec3 clusterColor()
{
	return HSVtoRGB(getHueValue(uint(vClusterIndex)), .99f, .99f);
}

void main() {
	color = colorUniform();
	gl_PointSize = pointSize;
	gl_Position = mModelViewProj * vPosition;
}