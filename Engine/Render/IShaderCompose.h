#pragma once

#include "Shader.h"

namespace platform::Render {

	class ShaderCompose {
	public:
		virtual ~ShaderCompose();

		static const white::uint8 NumTypes = (white::uint8)ShaderType::ComputeShader + 1;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
	};

	class GraphicsBuffer;
}