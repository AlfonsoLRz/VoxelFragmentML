#pragma once

#include "assimp/scene.h"
#include "Graphics/Application/GraphicsAppEnumerations.h"
#include "Graphics/Core/ComputeShader.h"
#include "Graphics/Core/GraphicsCoreEnumerations.h"
#include "Graphics/Core/RenderingShader.h"
#include "Graphics/Core/Texture.h"

/**
*	@file Material.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/19/2019
*/

/**
*	@brief Set of textures which defines the appearance of an object.
*/
class Material
{
protected:
	const static float DISPLACEMENT;
	const static float SHININESS;

public:
	struct MaterialDescription
	{
		std::string					_rootFolder;
		std::string					_name;
		std::vector<std::string>	_textureImage;
		std::vector<vec4>			_textureColor;
		float						_ns;

		MaterialDescription() : _textureImage(Texture::NUM_TEXTURE_TYPES), _textureColor(Texture::NUM_TEXTURE_TYPES), _rootFolder(""), _ns(1.0f)
		{
			for (size_t idx = 0; idx < _textureColor.size(); ++idx) _textureColor[idx] = vec4(-1.0f);
		}

		void setRootFolder(const std::string& path)
		{
			_rootFolder = path;
		}

		void setTextureColor(Texture::TextureTypes texture, const vec4& color)
		{
			_textureColor[texture] = color;
		}

		void setTexturePath(Texture::TextureTypes texture, const std::string& path)
		{
			_textureImage[texture] = path;
		}

		bool operator==(const MaterialDescription& other) const
		{
			bool equal = _rootFolder == other._rootFolder && _name == other._name && glm::epsilonEqual<float>(_ns, other._ns, glm::epsilon<float>());
			for (size_t idx = 0; idx < _textureColor.size(); ++idx) equal &= glm::distance(_textureColor[idx], other._textureColor[idx]) < glm::epsilon<float>();
			for (size_t idx = 0; idx < _textureImage.size(); ++idx) equal &= _textureImage[idx] == other._textureImage[idx];

			return equal;
		}
	};

protected:
	Texture*	_texture[Texture::NUM_TEXTURE_TYPES];						//!< Texture pointer for each type. Noone of them should exclude others
	float		_displacementFactor;										//!< Displacement along tangent
	float		_shininess;													//!< Phong exponent for specular reflection	

protected:
	/**
	*	@brief Copies the members of a material from this one.
	*	@param material Material source.
	*/
	void copyAttributes(const Material& material);

public:
	/**
	*	@brief Default constructor.
	*/
	Material();

	/**
	*	@brief Copy constructor.
	*	@param material Parameters source.
	*/
	Material(const Material& material);

	/**
	*	@brief Assignment operator overriding.
	*	@param material Parameters source.
	*/
	Material& operator=(const Material& material);

	/**
	*	@brief Specifies the values of uniform variables from the shader.
	*	@param shader Destiny of values to be specified.
	*/
	void applyMaterial(RenderingShader* shader);

	/**
	*	@brief Specifies the values of uniform variables from the shader.
	*	@param shader Destiny of values to be specified.
	*/
	void applyMaterial4ComputeShader(ComputeShader* shader, bool applyBump = false);

	/**
	*	@brief Applies individual textures. 
	*/
	bool applyTexture(ShaderProgram* shader, const Texture::TextureTypes textureType);

	/**
	*	@brief Transforms the assimp material into our representation.
	*/
	static MaterialDescription getMaterialDescription(aiMaterial* material, const std::string& folder);

	/**
	*	@brief Modifies the displacement along the points tangents.
	*	@param dispFactor Displacement length.
	*/
	void setDisplacementFactor(const float dispFactor);
	
	/**
	*	@brief Modifies the Phong exponent of material.
	*	@param shininess New shininess exponent.
	*/
	void setShininess(const float shininess);

	/**
	*	@brief Modifies the texture pointer for a certain type.
	*	@param textureType Texture slot.
	*	@param texture New texture.
	*/
	void setTexture(const Texture::TextureTypes textureType, Texture* texture);

public:
	struct MaterialSpecs
	{
		int16_t _texture[Texture::NUM_TEXTURE_TYPES];
		float _displacement;
		float _shininess;

		/**
		*	@brief Default constructor.
		*/
		MaterialSpecs()
		{
			for (int i = 0; i < Texture::NUM_TEXTURE_TYPES; ++i)
			{
				_texture[i] = -1;
			}

			_displacement = 0.0f;
			_shininess = 1.0f;
		}
	};
};

