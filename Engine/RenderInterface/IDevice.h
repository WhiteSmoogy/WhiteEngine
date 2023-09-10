/*! \file Engine\Render\IDevice.h
\ingroup Engine
\brief 创建接口类。
*/
#ifndef WE_RENDER_IDevice_h
#define WE_RENDER_IDevice_h 1

#include <WBase/wdef.h>
#include "DeviceCaps.h"
#include "ITexture.hpp"
#include "IGraphicsBuffer.hpp"
#include "InputLayout.hpp"
#include "IGPUResourceView.h"
#include "PipleState.h"
#include "Shader.h"
#include "IGraphicsPipelineState.h"
#include "IShaderCompose.h"
#include "RenderObject.h"
#include "DStorage.h"
#include "Core/Hash/MessageDigest.h"
#include <optional>

namespace asset {
	class ShaderBlobAsset;
}

namespace platform::Render::Effect {
	class Effect;
	class Technique;
}

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

	class GraphicsPipelineStateInitializer
	{
	public:
		using RenderTargetFormatsType = std::array<EFormat, MaxSimultaneousRenderTargets>;

		ShaderPassInput ShaderPass;

		RasterizerDesc RasterizerState;
		DepthStencilDesc DepthStencilState;
		BlendDesc BlendState;

		PrimtivteType Primitive;
	public:
		GraphicsPipelineStateInitializer()
		{
			RenderTargetFormats.fill(EF_Unknown);
		}

		uint32 GetNumRenderTargets() const
		{
			if (RenderTargetsEnabled > 0)
			{
				int32 LastValidTarget = -1;
				for (int32 i = (int32)RenderTargetsEnabled - 1; i >= 0; i--)
				{
					if (RenderTargetFormats[i] != EF_Unknown)
					{
						LastValidTarget = i;
						break;
					}
				}
				return uint32(LastValidTarget + 1);
			}

			return RenderTargetsEnabled;
		}

		uint32						RenderTargetsEnabled = 0;
		RenderTargetFormatsType		RenderTargetFormats;

		EFormat						DepthStencilTargetFormat;

		uint16						NumSamples = 1;
	};

	struct Texture3DInitializer
	{
		uint16 Width; 
		uint16 Height; 
		uint16 Depth; 
		uint8 NumMipmaps; 
		uint8 ArraySize;
		EFormat Format; 
		uint32 Access; 

		uint32 NumSamples = 1;
	};

	enum TextureCreateFlags
	{
		TexCreate_None = 0,
	};

	using DStorage = platform_ex::DirectStorage;

	class Device {
	public:
		virtual Texture1D* CreateTexture(uint16 width, uint8 num_mipmaps, uint8 array_size,
			EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const *> init_data = nullptr) = 0;

		virtual Texture2D* CreateTexture(uint16 width, uint16 height, uint8 num_mipmaps, uint8 array_size,
			EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const *>  init_data = nullptr) = 0;

		virtual Texture3D* CreateTexture(const Texture3DInitializer& Initializer, std::optional<ElementInitData const *>  init_data = nullptr) = 0;

		virtual TextureCube* CreateTextureCube(uint16 size, uint8 num_mipmaps, uint8 array_size,
			EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const *>  init_data = nullptr) = 0;

		virtual ShaderCompose* CreateShaderCompose(std::unordered_map<ShaderType,const asset::ShaderBlobAsset*> pShaderBlob, Effect::Effect* pEffect) = 0;

		virtual ConstantBuffer* CreateConstantBuffer(Buffer::Usage usage,uint32 size_in_byte,const void*  init_data = nullptr) = 0;

		virtual GraphicsBuffer* CreateBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, uint32 stride, std::optional<void const*>  init_data = nullptr) = 0;

		virtual GraphicsBuffer* CreateVertexBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, EFormat format, std::optional<void const *>  init_data = nullptr) = 0;
		virtual GraphicsBuffer* CreateIndexBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, EFormat format, std::optional<void const *>  init_data = nullptr) = 0;

		virtual PipleState* CreatePipleState(const PipleState& state) = 0;

		virtual InputLayout* CreateInputLayout() = 0;

		virtual UnorderedAccessView* CreateUnorderedAccessView(Texture2D* InTexture) =0;

		virtual GraphicsPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateInitializer& initializer) =0;
		virtual ComputePipelineState* CreateComputePipelineState(const platform::Render::ComputeHWShader* ComputeShader) = 0;

		virtual HardwareShader* CreateVertexShader(const white::span<const uint8>& Code) = 0;
		virtual HardwareShader* CreatePixelShader(const white::span<const uint8>& Code) = 0;
		virtual HardwareShader* CreateGeometryShader(const white::span<const uint8>& Code) = 0;
		virtual HardwareShader* CreateComputeShader(const white::span<const uint8>& Code) = 0;

		virtual SRVRIRef CreateShaderResourceView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format = EF_Unknown) = 0;
		virtual UAVRIRef CreateUnorderedAccessView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format = EF_Unknown) = 0;

		virtual DStorage& GetDStorage() = 0;
	};
}

#endif