#pragma once

#include "RenderInterface/InputLayout.hpp"
#include "d3d12_dxgi.h"

namespace platform_ex::Windows::D3D12
{
	using VertexElementsType = std::vector<D3D12_INPUT_ELEMENT_DESC>;

	class VertexDeclaration
	{
	public:
		VertexElementsType VertexElements;

		explicit VertexDeclaration(const VertexElementsType& InElements)
			:VertexElements(InElements)
		{}
	};

	VertexDeclaration* CreateVertexDeclaration(const platform::Render::VertexDeclarationElements& Elements);

	const char* SemanticName(platform::Render::Vertex::Usage usage);
}