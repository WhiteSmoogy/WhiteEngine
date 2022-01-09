#pragma once
#include "RenderInterface/Color_T.hpp"
#include "RenderInterface/ICommandList.h"
#include "Runtime/SceneInfo.h"


namespace WhiteEngine
{
	struct GizmosElementVertex
	{
		wm::float4 Position;
		wm::float2 TextureCoordinate;
		LinearColor Color;

		static platform::Render::VertexDeclarationElements VertexDeclaration;

		GizmosElementVertex(wm::float3 pos,wm::float2 uv,LinearColor color)
			:Position(pos,1), TextureCoordinate(uv),Color(color)
		{}
	};
	
	class GizmosElements
	{
	public:
		/** Adds a line to the batch. Note only SE_BLEND_Opaque will be used for batched line rendering. */
		void AddLine(const wm::float3& Start, const wm::float3& End, const LinearColor& Color, float Thickness = 0.0f, float DepthBias = 0.0f, bool bScreenSpace = false);

		bool Draw(platform::Render::CommandList& CmdList, const WhiteEngine::SceneInfo& Info);
	public:
		void AddSphere(wm::float3 Center, float Radius, const LinearColor& Color, int32 Segments = 16, float Thickness = 0.0f);
	private:
		struct BatchPoint
		{
			wm::float3 Position;
			float Size;
			FColor Color;
		};

		std::vector< GizmosElementVertex> Lines;
	};
}