#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 2) in vec2 vTextCoord;
layout(location = 4) in vec3 vOffset;
layout(location = 5) in vec3 vScale;

// Matrices
uniform mat4 mModelView;
uniform mat4 mModelViewProj;

out vec3 position;
out vec2 textCoord;


void main()
{
	mat4 scaleMatrix = mat4(vScale.x, .0f, .0f, .0f, .0f, vScale.y, .0f, .0f, .0f, .0f, vScale.z, .0f, .0f, .0f, .0f, 1.0f);
	mat4 translationMatrix = mat4(1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f, vOffset.x, vOffset.y, vOffset.z, 1.0f);
	vec3 newPosition = vec3(translationMatrix * scaleMatrix * vec4(vPosition, 1.0f));

	position = vec3(mModelView * vec4(newPosition, 1.0f));
	textCoord = vTextCoord;

	gl_Position = mModelViewProj * vec4(newPosition, 1.0f);
}
