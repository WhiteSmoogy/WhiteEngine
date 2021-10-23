#pragma once

#include "WBase/wmathtype.hpp"
#include "IGraphicsBuffer.hpp"
#include "InputLayout.hpp"
#include <memory>

namespace platform::Render {
	/** The vertex data used to filter a texture. */
	struct FilterVertex
	{
	public:
		white::math::float4 Position;
		white::math::float2 UV;
	};

	VertexDeclarationElements GFilterVertexDeclaration();

	std::shared_ptr<Render::GraphicsBuffer> GScreenRectangleVertexBuffer();

	std::shared_ptr<Render::GraphicsBuffer> GScreenRectangleIndexBuffer();
}