#pragma once

#include "ShaderParamterTraits.hpp"
#include "RenderInterface/ITexture.hpp"
#include "RenderInterface/TextureSampleDesc.h"

namespace platform::Render
{
	inline namespace Shader
	{
		template<typename TypeParameter>
		struct TShaderTextureTypeInfo;

		template<>
		struct TShaderTextureTypeInfo<Texture2D> : ShaderTypeInfo<SPT_texture2D>
		{
			using DeclType = Texture2D*;

			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = sizeof(DeclType);
		};

		template<>
		struct TShaderTextureTypeInfo<Texture3D> : ShaderTypeInfo<SPT_texture3D>
		{
			using DeclType = Texture3D*;

			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = sizeof(DeclType);
		};

		template<>
		struct TShaderParameterTypeInfo<TextureSampleDesc> : ShaderTypeInfo<SPT_sampler>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = 4;

			using DeclType = TextureSampleDesc;
		};
	}
}

