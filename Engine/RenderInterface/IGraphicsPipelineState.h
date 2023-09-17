#pragma once

#include "DeviceCaps.h"
#include "VertexDecl.h"
#include "RenderObject.h"
#include "Core/Hash/MessageDigest.h"

namespace platform::Render {
	class HardwareShader : public RObject
	{
	public:
		virtual ~HardwareShader() = 0;

		void SetHash(Digest::SHAHash InHash) { Hash = InHash; }
		Digest::SHAHash GetHash() const { return Hash; }
	private:
		Digest::SHAHash Hash;
	};

	class VertexHWShader : public HardwareShader
	{};

	class PixelHWShader : public HardwareShader
	{};

	class GeometryHWShader : public HardwareShader
	{};

	class ComputeHWShader : public HardwareShader
	{};

	template<typename T>
	concept THardwareShader = std::is_base_of_v<HardwareShader, T>;


	struct ShaderPassInput
	{
		VertexDeclarationElements VertexDeclaration;

		VertexHWShader* VertexShader = nullptr;
		PixelHWShader* PixelShader = nullptr;
		GeometryHWShader* GeometryShader = nullptr;

		HardwareShader* HullShader = nullptr;
		HardwareShader* DomainShader = nullptr;
	};

	class RootSignature
	{
	public:
		virtual ~RootSignature();
	};

	class PipelineStateObject
	{
	public:
		virtual ~PipelineStateObject();

		virtual const RootSignature* GetRootSignature() const = 0;
	};

	class GraphicsPipelineState :public PipelineStateObject
	{};

	class ComputePipelineState:public PipelineStateObject
	{
	public:
	};

	enum class DescriptorHeapType : uint8
	{
		Standard,
		Sampler,
		RenderTarget,
		DepthStencil,
		Count,
	};
}