#include "GizmosElements.h"
#include "RenderInterface/PipelineStateUtility.h"
#include "Runtime/RenderCore/ShaderParamterTraits.hpp"
#include "Runtime/RenderCore/ShaderParameterStruct.h"
#include "RenderInterface/BuiltInShader.h"

using namespace WhiteEngine;
using namespace platform::Render;

platform::Render::VertexDeclarationElements GizmosElementVertex::VertexDeclaration
{
	CtorVertexElement(0,woffsetof(GizmosElementVertex,Position),Vertex::Usage::Position,0,EF_ABGR32F,sizeof(GizmosElementVertex)),
	CtorVertexElement(0,woffsetof(GizmosElementVertex,TextureCoordinate),Vertex::Usage::TextureCoord,0,EF_GR32F,sizeof(GizmosElementVertex)),
	CtorVertexElement(0,woffsetof(GizmosElementVertex,Color),Vertex::Usage::Diffuse,0,EF_ABGR32F,sizeof(GizmosElementVertex)),
};

void GizmosElements::AddLine(const wm::float3& Start, const wm::float3& End, const LinearColor& Color, float Thickness, float DepthBias, bool bScreenSpace)
{
	LinearColor OpaqueColor(Color);
	OpaqueColor.a = 1;

	if (Thickness == 0.f)
	{
		if (DepthBias == 0.f)
		{
			Lines.emplace_back(Start, wm::float2(), Color);
			Lines.emplace_back(End,wm::float2(), Color);
		}
		else
		{
			//TODO
		}
	}
	else
	{
		//TODO
	}
}

class GizmosElementVertexShader : public BuiltInShader
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER(wm::float4x4, Transform)
		END_SHADER_PARAMETER_STRUCT();

	void SetParameters(CommandList& CmdList,ShaderRef<GizmosElementVertexShader> This, wm::float4x4 Transform)
	{
		Parameters Parameters;
		Parameters.Transform = wm::transpose(Transform);

		SetShaderParameters(CmdList, This, This.GetVertexShader(), Parameters);
	}

	EXPORTED_BUILTIN_SHADER(GizmosElementVertexShader);
};

class GizmosPixelShader : public BuiltInShader
{
public:
	EXPORTED_BUILTIN_SHADER(GizmosPixelShader);
};

IMPLEMENT_BUILTIN_SHADER(GizmosElementVertexShader, "GizmosElementsVertexShader.wsl", "Main", platform::Render::VertexShader);
IMPLEMENT_BUILTIN_SHADER(GizmosPixelShader, "GizmosPixelShader.wsl", "Main", platform::Render::PixelShader);

bool GizmosElements::Draw(platform::Render::CommandList& CmdList, const WhiteEngine::SceneInfo& Info)
{
	const auto& Transform = Info.Matrices.GetViewProjectionMatrix();

	GraphicsPipelineStateInitializer psoInit;
	CmdList.FillRenderTargetsInfo(psoInit);

	psoInit.RasterizerState.cull = CullMode::None;

	if (!Lines.empty())
	{
		psoInit.Primitive = PrimtivteType::LineList;

		//shaderPass
		psoInit.ShaderPass.VertexDeclaration = GizmosElementVertex::VertexDeclaration;

		auto VertexShader = GetBuiltInShaderMap()->GetShader< GizmosElementVertexShader>();
		psoInit.ShaderPass.VertexShader = VertexShader.GetVertexShader();
		psoInit.ShaderPass.PixelShader = GetBuiltInShaderMap()->GetShader<GizmosPixelShader>().GetPixelShader();

		SetGraphicsPipelineState(CmdList, psoInit);
		
		VertexShader->SetParameters(CmdList, VertexShader,Transform);

		auto VertexBuffer = CreateVertexBuffer(white::make_const_span((std::byte*)Lines.data(), Lines.size() * sizeof(GizmosElementVertex)),
			Buffer::Usage::Static,white::underlying(EAccessHint::VertexOrIndexBuffer));

		CmdList.SetVertexBuffer(0, VertexBuffer);

		int32 MaxVerticesAllowed = std::numeric_limits<int32>::max() / sizeof(GizmosElementVertex);

		int32 MinVertex = 0;
		int32 TotalVerts = (static_cast<uint32>(Lines.size()) / 2) * 2;

		while (MinVertex < TotalVerts)
		{
			int32 NumLinePrims = std::min(MaxVerticesAllowed, TotalVerts - MinVertex) / 2;
			CmdList.DrawPrimitive(MinVertex,0, NumLinePrims, 1);
			MinVertex += NumLinePrims * 2;
		}

		VertexBuffer->Release();
	}

	return true;
}

void GizmosElements::AddSphere(wm::float3 Center, float Radius, const LinearColor& Color, int32 Segments, float Thickness)
{
	Segments = std::max(Segments, 4);

	const float AngleInc = 2.f * wm::PI / Segments;

	auto SegementsY = Segments;
	float Latitude = AngleInc;
	float Longitude;
	float SinY1 = 0.0f, CosY1 = 1.0f, SinY2, CosY2;
	float SinX, CosX;

	while (SegementsY--)
	{
		SinY2 = std::sin(Latitude);
		CosY2 = std::cos(Latitude);

		auto Vertex1 = wm::float3(SinY1, CosY1, 0.0f) * Radius + Center;
		auto Vertex3 = wm::float3(SinY2, CosY2, 0.0f) * Radius + Center;
		Longitude = AngleInc;

		auto SegementsX = Segments;
		while (SegementsX--)
		{
			SinX = std::sin(Longitude);
			CosX = std::cos(Longitude);

			auto Vertex2 = wm::float3((CosX * SinY1), CosY1, (SinX * SinY1)) * Radius + Center;
			auto Vertex4 = wm::float3((CosX * SinY2), CosY2, (SinX * SinY2)) * Radius + Center;

			AddLine(Vertex1, Vertex2, Color, Thickness);
			AddLine(Vertex1, Vertex3, Color, Thickness);

			Vertex1 = Vertex2;
			Vertex3 = Vertex4;
			Longitude += AngleInc;
		}
		SinY1 = SinY2;
		CosY1 = CosY2;
		Latitude += AngleInc;
	}
}

void GizmosElements::AddAABB(wm::float3 Min, wm::float3 Max, const LinearColor& Color, float Thickness)
{
	wm::float3	B[2], P, Q;
	int32 i, j;

	B[0] = Min;
	B[1] = Max;

	for (i = 0; i < 2; i++) for (j = 0; j < 2; j++)
	{
		P.x = B[i].x; Q.x = B[i].x;
		P.y = B[j].y; Q.y = B[j].y;
		P.z = B[0].z; Q.z = B[1].z;
		AddLine(P, Q, Color, Thickness);

		P.y = B[i].y; Q.y = B[i].y;
		P.z = B[j].z; Q.z = B[j].z;
		P.x = B[0].x; Q.x = B[1].x;
		AddLine(P, Q, Color, Thickness);

		P.z = B[i].z; Q.z = B[i].z;
		P.x = B[j].x; Q.x = B[j].x;
		P.y = B[0].y; Q.y = B[1].y;
		AddLine(P, Q, Color, Thickness);
	}
}
