#pragma once

#define DEBUG_UNIFORMS false

class ShaderProgram
{
public:
	enum ShaderTypes : uint8_t
	{
		VERTEX_SHADER,
		FRAGMENT_SHADER,
		GEOMETRY_SHADER,
		COMPUTE_SHADER
	};

protected:
	const static std::string MODULE_HEADER;
	const static std::string MODULE_FILE_CHAR_1;
	const static std::string MODULE_FILE_CHAR_2;

protected:
	static std::unordered_map<std::string, std::string> _moduleCode;			//!< Modules that are already loaded

protected:
	std::vector<GLuint> _activeSubroutineUniform[COMPUTE_SHADER + 1];			//!< Active uniform for each subroutine for each shader type
	GLuint				_handler;												//!< Shader program id in GPU
	bool				_linked;												//!< Flag which tell us if the shader has been linked correctly
	std::string			_logString;												//!< Error message got from the last operation with the shader

protected:
	virtual GLuint	compileShader(const char* filename, const GLenum shaderType);
	bool			fileExists(const std::string& fileName);
	ShaderTypes		fromOpenGLToShaderTypes(const GLenum shaderType);
	bool			includeLibraries(std::string& shaderContent);
	virtual bool	loadFileContent(const std::string& filename, std::string& content);
	bool			showErrorMessage(const std::string& variableName);

public:
	ShaderProgram();
	virtual ~ShaderProgram();

	virtual void	applyActiveSubroutines() = 0;
	virtual GLuint	createShaderProgram(const char* filename) = 0;

	bool setSubroutineUniform(const GLenum shaderType, const std::string& subroutine, const std::string& functionName);
	bool setUniform(const std::string& name, const GLfloat value);
	bool setUniform(const std::string& name, const GLint value);
	bool setUniform(const std::string& name, const GLuint value);
	bool setUniform(const std::string& name, const mat4& value);
	bool setUniform(const std::string& name, const std::vector<mat4>& values);
	bool setUniform(const std::string& name, const vec2& value);
	bool setUniform(const std::string& name, const ivec2& value);
	bool setUniform(const std::string& name, const uvec2& value);
	bool setUniform(const std::string& name, const ivec3& value);
	bool setUniform(const std::string& name, const uvec3& value);
	bool setUniform(const std::string& name, const vec3& value);
	bool setUniform(const std::string& name, const vec4& value);
	bool setUniform(const std::string& name, const std::vector<float>& values);

	bool use();
};

