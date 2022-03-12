#include <WBase/smart_ptr.hpp>
#include <WFramework/WCLib/Debug.h>
#include <WFramework/WCLib/Logger.h>
#include <WBase/enum.hpp>

#include "../Effect/CopyEffect.h"
#include "spdlog/spdlog.h"

#include "NodeDevice.h"
#include "Adapter.h"
#include "Convert.h"
#include "Texture.h"
#include "RootSignature.h"
#include "HardwareShader.h"
#include "Utility.h"

namespace Vertex = platform::Render::Vertex;
namespace Buffer = platform::Render::Buffer;

using std::make_shared;

namespace platform_ex::Windows::D3D12 {
	Device::Device(DXGI::Adapter & InAdapter)
		:adapter(InAdapter),
		PipelineStateCache(this),
		bHeapNotZeroedSupported(false)
	{
		std::vector<D3D_FEATURE_LEVEL> feature_levels = {
			D3D_FEATURE_LEVEL_12_2 ,
			D3D_FEATURE_LEVEL_12_1 ,
			D3D_FEATURE_LEVEL_12_0 ,
			D3D_FEATURE_LEVEL_11_1 ,
			D3D_FEATURE_LEVEL_11_0 };

		for (auto level : feature_levels) {
			ID3D12Device* device = nullptr;
			if (SUCCEEDED(D3D12::CreateDevice(adapter.Get(),
				level, IID_ID3D12Device, reinterpret_cast<void**>(&device)))) {

#if 1
				COMPtr<ID3D12InfoQueue> info_queue;
				if (SUCCEEDED(device->QueryInterface(COMPtr_RefParam(info_queue, IID_ID3D12InfoQueue)))) {
					D3D12_INFO_QUEUE_FILTER filter;
					ZeroMemory(&filter, sizeof(filter));

					D3D12_MESSAGE_SEVERITY denySeverity = D3D12_MESSAGE_SEVERITY_INFO;
					filter.DenyList.NumSeverities = 1;
					filter.DenyList.pSeverityList = &denySeverity;

					D3D12_MESSAGE_ID denyIds[] =
					{
						//      This warning gets triggered by ClearDepthStencilView/ClearRenderTargetView because when the resource was created
						//      it wasn't passed an optimized clear color (see CreateCommitedResource). This shows up a lot and is very noisy.
						D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
						D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
					};
					filter.DenyList.NumIDs = sizeof(denyIds) / sizeof(D3D12_MESSAGE_ID);
					filter.DenyList.pIDList = denyIds;

					CheckHResult(info_queue->PushStorageFilter(&filter));

					info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				}
#endif

				D3D12_FEATURE_DATA_FEATURE_LEVELS req_feature_levels;
				req_feature_levels.NumFeatureLevels = static_cast<UINT>(feature_levels.size());
				req_feature_levels.pFeatureLevelsRequested = &feature_levels[0];
				device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &req_feature_levels, sizeof(req_feature_levels));

				DeviceEx(device, nullptr, req_feature_levels.MaxSupportedFeatureLevel);
				CheckFeatureSupport(device);

				auto desc = adapter.Description();
				char const * feature_level_str;
				switch (req_feature_levels.MaxSupportedFeatureLevel)
				{
				case D3D_FEATURE_LEVEL_12_1:
					feature_level_str = " D3D_FEATURE_LEVEL_12_1";
					break;

				case D3D_FEATURE_LEVEL_12_0:
					feature_level_str = " D3D_FEATURE_LEVEL_12_0";
					break;

				case D3D_FEATURE_LEVEL_11_1:
					feature_level_str = " D3D_FEATURE_LEVEL_11_0";
					break;

				case D3D_FEATURE_LEVEL_11_0:
					feature_level_str = " D3D_FEATURE_LEVEL_11_1";
					break;

				default:
					feature_level_str = " D3D_FEATURE_LEVEL_UN_0";
					break;
				}
				WF_Trace(platform::Descriptions::Notice, "%s Adapter Description:%s", wfname, desc.c_str());

				//todo if something

				break;
			}
		}

	}

	Texture1D* Device::CreateTexture(uint16 width, uint8 num_mipmaps, uint8 array_size, EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const *>  init_data)
	{
		auto texture = std::make_unique<Texture1D>(width, num_mipmaps, array_size, format, access, sample_info);
		if (init_data.has_value())
			texture->HWResourceCreate(init_data.value());
		return texture.release();
	}

	Texture2D* Device::CreateTexture(uint16 width, uint16 height, uint8 num_mipmaps, uint8 array_size, EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const *>  init_data)
	{
		auto texture = std::make_unique<Texture2D>(width, height, num_mipmaps, array_size, format, access, sample_info);
		if (init_data.has_value())
			texture->HWResourceCreate(init_data.value());

		if ((access & platform::Render::EA_RTV) == platform::Render::EA_RTV)
		{
			texture->SetNumRenderTargetViews(1);

			uint32 RTVIndex = 0;

			// Create a render-target-view for the texture.
			D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
			std::memset(&RTVDesc, 0, sizeof(RTVDesc));
			RTVDesc.Format = texture->GetDXGIFormat();
			RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			RTVDesc.Texture2D.MipSlice = 0;
			RTVDesc.Texture2D.PlaneSlice = 0;

			texture->SetRenderTargetViewIndex(new RenderTargetView(GetDefaultNodeDevice(), RTVDesc, *texture), RTVIndex);
		}

		if ((access & platform::Render::EA_DSV) == platform::Render::EA_DSV)
		{
			// Create a depth-stencil-view for the texture.
			D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
			DSVDesc.Format = platform_ex::Windows::D3D12::FindDepthStencilDXGIFormat(texture->GetDXGIFormat());
			DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			DSVDesc.Texture2D.MipSlice = 0;

			const bool HasStencil = IsStencilFormat(format);

			for (uint32 AccessType = 0; AccessType != platform::Render::ExclusiveDepthStencil::MaxIndex; ++AccessType)
			{
				DSVDesc.Flags = D3D12_DSV_FLAG_NONE;

				texture->SetDepthStencilView(new DepthStencilView(GetDefaultNodeDevice(), DSVDesc, *texture, HasStencil), AccessType);
			}
		}

		return texture.release();
	}

	Texture3D* Device::CreateTexture(const platform::Render::Texture3DInitializer& Initializer,  std::optional<ElementInitData const *>  init_data)
	{
		auto texture = std::make_unique<Texture3D>(Initializer.Width, Initializer.Height, Initializer.Depth, Initializer.NumMipmaps, Initializer.ArraySize, Initializer.Format, Initializer.Access, Initializer.NumSamples);
		if (init_data.has_value())
			texture->HWResourceCreate(init_data.value());

		if ((Initializer.Access & platform::Render::EA_RTV) == platform::Render::EA_RTV)
		{
			texture->SetNumRenderTargetViews(1);

			uint32 RTVIndex = 0;

			// Create a render-target-view for the texture.
			D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
			std::memset(&RTVDesc,0, sizeof(RTVDesc));
			RTVDesc.Format = texture->GetDXGIFormat();
			RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			RTVDesc.Texture3D.MipSlice = 0;
			RTVDesc.Texture3D.FirstWSlice = 0;
			RTVDesc.Texture3D.WSize = Initializer.Depth;

			texture->SetRenderTargetViewIndex(new RenderTargetView(GetDefaultNodeDevice(), RTVDesc, *texture),RTVIndex);
		}

		return texture.release();
	}

	TextureCube* Device::CreateTextureCube(uint16 size, uint8 num_mipmaps, uint8 array_size, EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const *>  init_data)
	{
		auto texture = std::make_unique<TextureCube>(size, num_mipmaps, array_size, format, access, sample_info);
		if (init_data.has_value())
			texture->HWResourceCreate(init_data.value());
		return texture.release();
	}

	ShaderCompose * platform_ex::Windows::D3D12::Device::CreateShaderCompose(std::unordered_map<platform::Render::ShaderType, const asset::ShaderBlobAsset*> pShaderBlob, platform::Render::Effect::Effect* pEffect)
	{
		return std::make_unique<ShaderCompose>(pShaderBlob, pEffect).release();
	}

	white::observer_ptr<RootSignature> Device::CreateRootSignature(const QuantizedBoundShaderState& QBSS)
	{
		return white::make_observer(root_signatures->GetRootSignature(QBSS));
	}

	PipleState * Device::CreatePipleState(const platform::Render::PipleState & state)
	{
		return std::make_unique<PipleState>(state).release();
	}

	InputLayout * Device::CreateInputLayout()
	{
		return std::make_unique<InputLayout>().release();
	}

	GraphicsPipelineState* Device::CreateGraphicsPipelineState(const platform::Render::GraphicsPipelineStateInitializer& initializer)
	{
		auto& PSOCache = PipelineStateCache;

		QuantizedBoundShaderState QBSS;
		QuantizeBoundShaderState(QBSS, initializer);

		auto RootSignature = CreateRootSignature(QBSS).get();

		KeyGraphicsPipelineStateDesc LowLevelDesc;

		auto Found = PSOCache.FindInLoadedCache(initializer, RootSignature, LowLevelDesc);

		if (Found)
		{
			return Found;
		}

		return PSOCache.CreateAndAdd(initializer, RootSignature, LowLevelDesc);
	}

	ComputePipelineState* D3D12::Device::CreateComputePipelineState(platform::Render::ComputeHWShader* ComputeShader)
	{
		auto& PSOCache = PipelineStateCache;

		KeyComputePipelineStateDesc LowLevelDesc;

		auto Found = PSOCache.FindInLoadedCache(static_cast<ComputeHWShader*>(ComputeShader), LowLevelDesc);

		if (Found)
		{
			return Found;
		}

		return PSOCache.CreateAndAdd(static_cast<ComputeHWShader*>(ComputeShader), LowLevelDesc);
	}

	UnorderedAccessView* Device::CreateUnorderedAccessView(platform::Render::Texture2D* InTexture)
	{
		return std::make_unique<UnorderedAccessView>(Devices[0],CreateUAVDesc(*InTexture, 0, InTexture->GetArraySize(), 0),static_cast<Texture2D&>(*InTexture)).release();
	}

	ConstantBuffer * Device::CreateConstantBuffer(Buffer::Usage usage, uint32 size_in_byte,const void* init_data)
	{
		wconstraint(size_in_byte > 0);
		wconstraint(Align(size_in_byte,16) == size_in_byte);
		usage = white::enum_and(usage ,Buffer::Usage::LifeTimeMask);
		auto buffer = new ConstantBuffer(GetNodeDevice(0), usage, size_in_byte);

		void* MappedData = nullptr;
		if (usage == Buffer::Usage::MultiFrame) {
			auto& Allocator = GetUploadHeapAllocator(0);

			MappedData = Allocator.AllocUploadResource(size_in_byte, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, buffer->Location);
		}
		else {
			auto& Allocator = GetTransientConstantBufferAllocator();

			MappedData = Allocator.Allocate(size_in_byte, buffer->Location);
		}
		wconstraint(buffer->Location.GetOffsetFromBaseOfResource() % 16 == 0);
		wconstraint(MappedData != nullptr);
		if (init_data != nullptr)
			std::memcpy(MappedData, init_data, size_in_byte);

		return buffer;
	}

	FastConstantAllocator& platform_ex::Windows::D3D12::Device::GetTransientConstantBufferAllocator()
	{
		static class TransientConstantBufferAllocator : public FastConstantAllocator
		{
		public:
			using FastConstantAllocator::FastConstantAllocator;
		} Alloc(GetNodeDevice(0),GPUMaskType::AllGPU());

		return Alloc;
	}

	ResourceCreateInfo FillResourceCreateInfo(std::optional<void const*> init_data,const char* DebugName)
	{
		ResourceCreateInfo CreateInfo;

		CreateInfo.WithoutNativeResource = !init_data.has_value();
		if (!CreateInfo.WithoutNativeResource)
			CreateInfo.ResouceData = *init_data;

		CreateInfo.DebugName = DebugName;

		return CreateInfo;
	}

	GraphicsBuffer * Device::CreateVertexBuffer(platform::Render::Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, EFormat format, std::optional<void const*> init_data)
	{
		auto CreateInfo = FillResourceCreateInfo(init_data, "VertexBuffer");

		auto vb = CreateBuffer(&platform::Render::CommandListExecutor::GetImmediateCommandList(), usage, access, size_in_byte, 0, Convert(format), CreateInfo);

		return vb;
	}

	GraphicsBuffer * Device::CreateIndexBuffer(platform::Render::Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, EFormat format, std::optional<void const*> init_data)
	{
		wconstraint(format == platform::Render::EFormat::EF_R16UI || format == platform::Render::EFormat::EF_R32UI);
		auto CreateInfo = FillResourceCreateInfo(init_data, "IndexBuffer");

		return CreateBuffer(&platform::Render::CommandListExecutor::GetImmediateCommandList(), usage, access, size_in_byte,NumFormatBytes(format), Convert(format), CreateInfo);
	}

	platform::Render::HardwareShader* Device::CreateShader(const white::span<const uint8>& Code, platform::Render::ShaderType Type)
	{
		switch (Type)
		{
		case platform::Render::VertexShader:
				return new VertexHWShader(Code);
		case platform::Render::PixelShader:
			return new PixelHWShader(Code);
		case platform::Render::GeometryShader:
			return new GeometryHWShader(Code);
		case platform::Render::ComputeShader:
		{
			ComputeHWShader* Shader = new ComputeHWShader(Code);
			QuantizedBoundShaderState QBSS;
			QuantizeBoundShaderState(GetResourceBindingTier(), Shader, QBSS);

			Shader->pRootSignature = root_signatures->GetRootSignature(QBSS);
			return Shader;
		}
		}
		WAssert(false, "unimpl shader type");
		return nullptr;
	}

	platform::Render::HardwareShader* Device::CreateVertexShader(const white::span<const uint8>& Code)
	{
		return CreateShader(Code, platform::Render::VertexShader);
	}
	platform::Render::HardwareShader* Device::CreatePixelShader(const white::span<const uint8>& Code)
	{
		return CreateShader(Code, platform::Render::PixelShader);
	}
	platform::Render::HardwareShader* Device::CreateGeometryShader(const white::span<const uint8>& Code)
	{
		return CreateShader(Code, platform::Render::GeometryShader);
	}
	platform::Render::HardwareShader* Device::CreateComputeShader(const white::span<const uint8>& Code) 
	{
		return CreateShader(Code, platform::Render::ComputeShader);
	}


	ID3D12Device*  Device::operator->() wnoexcept {
		return d3d_device.Get();
	}

	white::observer_ptr<ID3D12DescriptorHeap> Device::CreateDynamicCBVSRVUAVDescriptorHeap(uint32_t num) {
		ID3D12DescriptorHeap* dynamic_heap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC cbv_srv_heap_desc;
		cbv_srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbv_srv_heap_desc.NumDescriptors = num;
		cbv_srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbv_srv_heap_desc.NodeMask = 0;
		CheckHResult(d3d_device->CreateDescriptorHeap(&cbv_srv_heap_desc, IID_ID3D12DescriptorHeap,white::replace_cast<void**>(&dynamic_heap)));
		return white::make_observer(dynamic_heap);
	}

	white::observer_ptr<ID3D12PipelineState> Device::CreateRenderPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		char const * p = reinterpret_cast<char const *>(&desc);
		auto hash_val = white::hash(p, p + sizeof(desc));

		auto iter = graphics_psos.find(hash_val);
		if (iter == graphics_psos.end())
		{
			ID3D12PipelineState* d3d_pso = nullptr;
			CheckHResult(d3d_device->CreateGraphicsPipelineState(&desc, IID_ID3D12PipelineState, white::replace_cast<void**>(&d3d_pso)));
			return white::make_observer(graphics_psos.emplace(hash_val,d3d_pso).first->second.Get());
		}
		else
		{
			return white::make_observer(iter->second.Get());
		}
	}

	void Device::DeviceEx(ID3D12Device * device, ID3D12CommandQueue * cmd_queue, D3D_FEATURE_LEVEL feature_level)
	{
		d3d_device = device;
		D3D::Debug(d3d_device, "Device");

		d3d_feature_level = feature_level;

		CheckHResult(d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			COMPtr_RefParam(d3d_cmd_allocators[Command_Resource], IID_ID3D12CommandAllocator)));
		D3D::Debug(d3d_cmd_allocators[Command_Resource], "Resource_Command_Allocator");

		auto create_desc_heap = [&](D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT NumDescriptors,
			D3D12_DESCRIPTOR_HEAP_FLAGS Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, UINT NodeMask = 0)
		{
			D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc = {
				Type,
				NumDescriptors,
				Flags,
				NodeMask
			};
			CheckHResult(d3d_device->CreateDescriptorHeap(&descriptor_desc,
				COMPtr_RefParam(d3d_desc_heaps[Type], IID_ID3D12DescriptorHeap)));
			d3d_desc_incres_sizes[Type] = d3d_device->GetDescriptorHandleIncrementSize(Type);
		};

		create_desc_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_MAX_RENDER_TARGET_VIEWS);
		create_desc_heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, NUM_MAX_DEPTH_STENCIL_VIEWS);
		create_desc_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NUM_MAX_CBV_SRV_UAVS);

		root_signatures = std::make_unique<RootSignatureMap>(device);

		FillCaps();

		Devices[0] = new NodeDevice(GPUMaskType::FromIndex(0), this);

		UploadHeapAllocators[0] = new UploadHeapAllocator(this, Devices[0], "Upload Buffer Allocator");

		PipelineStateCache.Init("", "", "");

		bool bRayTracingSupported = false;
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 Features = {};
			if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features, sizeof(Features)))
				&& Features.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
			{
				bRayTracingSupported = true;
			}
		}

		if (bRayTracingSupported)
		{
			d3d_device->QueryInterface(IID_PPV_ARGS(raytracing_device.ReleaseAndGetAddress()));
		}
		else
		{
			spdlog::error("ERROR: DirectX Raytracing is not supported by your OS, GPU and/or driver.");
		}

		InitializeRayTracing();

		frame_fence = new ManualFence(this, GPUMaskType::AllGPU(), "Adapter Frame Fence");
		frame_fence->CreateFence();

#if  ENABLE_AFTER_MATH
		if (GEnableNvidaiAfterMath)
		{
			uint32 Flags = GFSDK_Aftermath_FeatureFlags_Minimum;

			Flags |= GFSDK_Aftermath_FeatureFlags_EnableMarkers;
			Flags |= GFSDK_Aftermath_FeatureFlags_CallStackCapturing;
			Flags |= GFSDK_Aftermath_FeatureFlags_EnableResourceTracking;
			Flags |= GFSDK_Aftermath_FeatureFlags_Maximum;

			GFSDK_Aftermath_Result Result = GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, (GFSDK_Aftermath_FeatureFlags)Flags, d3d_device.Get());
			if (Result == GFSDK_Aftermath_Result_Success)
			{
				spdlog::info("[Aftermath] Aftermath enabled and primed");
			}
			else
			{
				spdlog::warn("[Aftermath] Aftermath enabled but failed to initialize ({})", white::underlying(Result));
				GEnableNvidaiAfterMath = 0;
			}
		}
#endif //  ENABLE_AFTER_MATH


		if (SUCCEEDED(d3d_device->QueryInterface(COMPtr_RefParam(d3d_debug_device, IID_ID3D12DebugDevice)))) {
			WAssertNonnull(d3d_debug_device);
		}
	}

	void platform_ex::Windows::D3D12::Device::InitializeRayTracing()
	{
		for (auto Device : Devices)
		{
			if (Device->GetRayTracingDevice())
			{
				Device->InitRayTracing();
			}
		}
	}

	void Device::FillCaps() {
		d3d_caps.type = platform::Render::Caps::Type::D3D12;
		d3d_caps.max_texture_depth = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;

		std::pair<EFormat, DXGI_FORMAT> fmts[] =
		{
			std::make_pair(EF_A8, DXGI_FORMAT_A8_UNORM),
			std::make_pair(EF_R5G6B5, DXGI_FORMAT_B5G6R5_UNORM),
			std::make_pair(EF_A1RGB5, DXGI_FORMAT_B5G5R5A1_UNORM),
			std::make_pair(EF_ARGB4, DXGI_FORMAT_B4G4R4A4_UNORM),
			std::make_pair(EF_R8, DXGI_FORMAT_R8_UNORM),
			std::make_pair(EF_SIGNED_R8, DXGI_FORMAT_R8_SNORM),
			std::make_pair(EF_GR8, DXGI_FORMAT_R8G8_UNORM),
			std::make_pair(EF_SIGNED_GR8, DXGI_FORMAT_R8G8_SNORM),
			std::make_pair(EF_ARGB8, DXGI_FORMAT_B8G8R8A8_UNORM),
			std::make_pair(EF_ABGR8, DXGI_FORMAT_R8G8B8A8_UNORM),
			std::make_pair(EF_SIGNED_ABGR8, DXGI_FORMAT_R8G8B8A8_SNORM),
			std::make_pair(EF_A2BGR10, DXGI_FORMAT_R10G10B10A2_UNORM),
			std::make_pair(EF_SIGNED_A2BGR10, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM),
			std::make_pair(EF_R8UI, DXGI_FORMAT_R8_UINT),
			std::make_pair(EF_R8I, DXGI_FORMAT_R8_SINT),
			std::make_pair(EF_GR8UI, DXGI_FORMAT_R8G8_UINT),
			std::make_pair(EF_GR8I, DXGI_FORMAT_R8G8_SINT),
			std::make_pair(EF_ABGR8UI, DXGI_FORMAT_R8G8B8A8_UINT),
			std::make_pair(EF_ABGR8I, DXGI_FORMAT_R8G8B8A8_SINT),
			std::make_pair(EF_A2BGR10UI, DXGI_FORMAT_R10G10B10A2_UINT),
			std::make_pair(EF_R16, DXGI_FORMAT_R16_UNORM),
			std::make_pair(EF_SIGNED_R16, DXGI_FORMAT_R16_SNORM),
			std::make_pair(EF_GR16, DXGI_FORMAT_R16G16_UNORM),
			std::make_pair(EF_SIGNED_GR16, DXGI_FORMAT_R16G16_SNORM),
			std::make_pair(EF_ABGR16, DXGI_FORMAT_R16G16B16A16_UNORM),
			std::make_pair(EF_SIGNED_ABGR16, DXGI_FORMAT_R16G16B16A16_SNORM),
			std::make_pair(EF_R16UI, DXGI_FORMAT_R16_UINT),
			std::make_pair(EF_R16I, DXGI_FORMAT_R16_SINT),
			std::make_pair(EF_GR16UI, DXGI_FORMAT_R16G16_UINT),
			std::make_pair(EF_GR16I, DXGI_FORMAT_R16G16_SINT),
			std::make_pair(EF_ABGR16UI, DXGI_FORMAT_R16G16B16A16_UINT),
			std::make_pair(EF_ABGR16I, DXGI_FORMAT_R16G16B16A16_SINT),
			std::make_pair(EF_R32UI, DXGI_FORMAT_R32_UINT),
			std::make_pair(EF_R32I, DXGI_FORMAT_R32_SINT),
			std::make_pair(EF_GR32UI, DXGI_FORMAT_R32G32_UINT),
			std::make_pair(EF_GR32I, DXGI_FORMAT_R32G32_SINT),
			std::make_pair(EF_BGR32UI, DXGI_FORMAT_R32G32B32_UINT),
			std::make_pair(EF_BGR32I, DXGI_FORMAT_R32G32B32_SINT),
			std::make_pair(EF_ABGR32UI, DXGI_FORMAT_R32G32B32A32_UINT),
			std::make_pair(EF_ABGR32I, DXGI_FORMAT_R32G32B32A32_SINT),
			std::make_pair(EF_R16F, DXGI_FORMAT_R16_FLOAT),
			std::make_pair(EF_GR16F, DXGI_FORMAT_R16G16_FLOAT),
			std::make_pair(EF_B10G11R11F, DXGI_FORMAT_R11G11B10_FLOAT),
			std::make_pair(EF_ABGR16F, DXGI_FORMAT_R16G16B16A16_FLOAT),
			std::make_pair(EF_R32F, DXGI_FORMAT_R32_FLOAT),
			std::make_pair(EF_GR32F, DXGI_FORMAT_R32G32_FLOAT),
			std::make_pair(EF_BGR32F, DXGI_FORMAT_R32G32B32_FLOAT),
			std::make_pair(EF_ABGR32F, DXGI_FORMAT_R32G32B32A32_FLOAT),
			std::make_pair(EF_BC1, DXGI_FORMAT_BC1_UNORM),
			std::make_pair(EF_BC2, DXGI_FORMAT_BC2_UNORM),
			std::make_pair(EF_BC3, DXGI_FORMAT_BC3_UNORM),
			std::make_pair(EF_BC4, DXGI_FORMAT_BC4_UNORM),
			std::make_pair(EF_SIGNED_BC4, DXGI_FORMAT_BC4_SNORM),
			std::make_pair(EF_BC5, DXGI_FORMAT_BC5_UNORM),
			std::make_pair(EF_SIGNED_BC5, DXGI_FORMAT_BC5_SNORM),
			std::make_pair(EF_BC6, DXGI_FORMAT_BC6H_UF16),
			std::make_pair(EF_SIGNED_BC6, DXGI_FORMAT_BC6H_SF16),
			std::make_pair(EF_BC7, DXGI_FORMAT_BC7_UNORM),
			std::make_pair(EF_D16, DXGI_FORMAT_D16_UNORM),
			std::make_pair(EF_D24S8, DXGI_FORMAT_D24_UNORM_S8_UINT),
			std::make_pair(EF_D32F, DXGI_FORMAT_D32_FLOAT),
			std::make_pair(EF_ARGB8_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB),
			std::make_pair(EF_ABGR8_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB),
			std::make_pair(EF_BC1_SRGB, DXGI_FORMAT_BC1_UNORM_SRGB),
			std::make_pair(EF_BC2_SRGB, DXGI_FORMAT_BC2_UNORM_SRGB),
			std::make_pair(EF_BC3_SRGB, DXGI_FORMAT_BC3_UNORM_SRGB),
			std::make_pair(EF_BC7_SRGB, DXGI_FORMAT_BC7_UNORM_SRGB)
		};

		std::vector<EFormat> vertex_support_formats;
		std::vector<EFormat> texture_support_formats;
		std::unordered_map<EFormat, std::vector<SampleDesc>> rt_support_msaas;
		D3D12_FEATURE_DATA_FORMAT_SUPPORT fmt_support;
		for (size_t i = 0; i < sizeof(fmts) / sizeof(fmts[0]); ++i)
		{
			DXGI_FORMAT dxgi_fmt;
			if (IsDepthFormat(fmts[i].first))
			{
				switch (fmts[i].first)
				{
				case EF_D16:
					dxgi_fmt = DXGI_FORMAT_R16_TYPELESS;
					break;

				case EF_D24S8:
					dxgi_fmt = DXGI_FORMAT_R24G8_TYPELESS;
					break;

				case EF_D32F:
				default:
					dxgi_fmt = DXGI_FORMAT_R32_TYPELESS;
					break;
				}

				fmt_support.Format = dxgi_fmt;
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER;
				fmt_support.Support2 = D3D12_FORMAT_SUPPORT2_NONE;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					vertex_support_formats.push_back(fmts[i].first);
				}

				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURE1D;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURE2D;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURE3D;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURECUBE;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_SHADER_LOAD;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
			}
			else
			{
				dxgi_fmt = fmts[i].second;

				fmt_support.Format = dxgi_fmt;
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER;
				fmt_support.Support2 = D3D12_FORMAT_SUPPORT2_NONE;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					vertex_support_formats.push_back(fmts[i].first);
				}

				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURE1D;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURE2D;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURE3D;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_TEXTURECUBE;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
				fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE;
				if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
				{
					texture_support_formats.push_back(fmts[i].first);
				}
			}

			bool rt_supported = false;
			fmt_support.Format = dxgi_fmt;
			fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_RENDER_TARGET;
			fmt_support.Support2 = D3D12_FORMAT_SUPPORT2_NONE;
			if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
			{
				rt_supported = true;
			}
			fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
			if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
			{
				rt_supported = true;
			}
			fmt_support.Support1 = D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL;
			if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fmt_support, sizeof(fmt_support))))
			{
				rt_supported = true;
			}

			if (rt_supported)
			{
				D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaa_quality_levels;
				msaa_quality_levels.Format = dxgi_fmt;

				UINT count = 1;
				while (count <= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)
				{
					msaa_quality_levels.SampleCount = count;
					if (SUCCEEDED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaa_quality_levels, sizeof(msaa_quality_levels))))
					{
						if (msaa_quality_levels.NumQualityLevels > 0)
						{
							rt_support_msaas[fmts[i].first].emplace_back(count, msaa_quality_levels.NumQualityLevels);
							count <<= 1;
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
			}
		}

		d3d_caps.TextureFormatSupport = [formats = std::move(texture_support_formats)](EFormat format) {
			return std::find(formats.begin(), formats.end(), format) != formats.end();
		};

		d3d_caps.VertexFormatSupport = [formats = std::move(vertex_support_formats)](EFormat format) {
			return std::find(formats.begin(), formats.end(), format) != formats.end();
		};

		d3d_caps.RenderTargetMSAASupport = [formats = std::move(rt_support_msaas)](EFormat format, SampleDesc sample) {
			auto iter = formats.find(format);
			if (iter != formats.end()) {
				for (auto msaa : iter->second) {
					if ((sample.Count == msaa.Count) && (sample.Quality < msaa.Quality)) {
						return true;
					}
				}
			}
			return false;
		};

		d3d_caps.support_hdr = [&]() {
			return adapter.CheckHDRSupport();
		}();
	}

	platform::Render::Caps& Device::GetCaps() {
		return d3d_caps;
	}

	platform::Render::Effect::CopyEffect& D3D12::Device::BiltEffect()
	{
		if (!bilt_effect)
			bilt_effect = std::make_unique<platform::Render::Effect::CopyEffect>("Copy");
		return Deref(bilt_effect);
	}

	platform::Render::InputLayout& D3D12::Device::PostProcessLayout()
	{
		if (!postprocess_layout)
		{
			postprocess_layout = std::make_unique<InputLayout>();
			postprocess_layout->SetTopoType(platform::Render::PrimtivteType::TriangleStrip);

			math::float2 postprocess_pos[] = {
				math::float2(-1,+1),
				math::float2(+1,+1),
				math::float2(-1,-1),
				math::float2(+1,-1),
			};

			postprocess_layout->BindVertexStream(share_raw(CreateVertexBuffer(Buffer::Usage::Static, EAccessHint::EA_GPURead | EAccessHint::EA_Immutable, sizeof(postprocess_pos), EFormat::EF_Unknown, postprocess_pos)), { Vertex::Element{ Vertex::Position,0,EFormat::EF_GR32F } });
		}
		return platform::Deref(postprocess_layout);
	}

	void D3D12::Device::CheckFeatureSupport(ID3D12Device* device)
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS D3D12Caps {};
		CheckHResult(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &D3D12Caps, sizeof(D3D12Caps)));
		ResourceHeapTier = D3D12Caps.ResourceHeapTier;
		ResourceBindingTier = D3D12Caps.ResourceBindingTier;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE D3D12RootSignatureCaps = {};
		D3D12RootSignatureCaps.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;	// This is the highest version we currently support. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &D3D12RootSignatureCaps, sizeof(D3D12RootSignatureCaps))))
		{
			D3D12RootSignatureCaps.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}
		RootSignatureVersion = D3D12RootSignatureCaps.HighestVersion;

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 Features = {};
		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features, sizeof(Features))))
		{
			bHeapNotZeroedSupported = true;
		}
	}
	 
	void D3D12::Device::EndFrame()
	{
		uint64 FrameLag = 2;
		GetUploadHeapAllocator(0).CleanUpAllocations(FrameLag);

		platform::Render::RObject::FlushPendingDeletes();
	}

	UploadHeapAllocator& D3D12::Device::GetUploadHeapAllocator(uint32 GPUIndex)
	{
		return *UploadHeapAllocators[GPUIndex];
	}

	HRESULT D3D12::Device::CreateBuffer(D3D12_HEAP_TYPE HeapType, GPUMaskType CreationNode, GPUMaskType VisibleNodes, uint64 HeapSize, ResourceHolder** ppOutResource, const char* Name, D3D12_RESOURCE_FLAGS Flags)
	{
		const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(HeapType, CreationNode.GetNative(), VisibleNodes.GetNative());
		const D3D12_RESOURCE_STATES InitialState = DetermineInitialResourceState(HeapProps.Type, &HeapProps);
		return CreateBuffer(HeapProps, CreationNode, InitialState, InitialState, HeapSize, ppOutResource, Name, Flags);
	}

	HRESULT D3D12::Device::CreateBuffer(const D3D12_HEAP_PROPERTIES& HeapProps, GPUMaskType CreationNode, D3D12_RESOURCE_STATES InitialState, D3D12_RESOURCE_STATES InDefaultState, uint64 HeapSize, ResourceHolder** ppOutResource, const char* Name, D3D12_RESOURCE_FLAGS Flags)
	{
		if (!ppOutResource)
		{
			return E_POINTER;
		}

		const D3D12_RESOURCE_DESC BufDesc = CD3DX12_RESOURCE_DESC::Buffer(HeapSize, Flags);

		return CreateCommittedResource(BufDesc, CreationNode, HeapProps, InitialState, InDefaultState, nullptr, ppOutResource, Name);
	}

	HRESULT D3D12::Device::CreateCommittedResource(const D3D12_RESOURCE_DESC& BufDesc, GPUMaskType CreationNode, const D3D12_HEAP_PROPERTIES& HeapProps, D3D12_RESOURCE_STATES InInitialState, D3D12_RESOURCE_STATES InDefaultState, const D3D12_CLEAR_VALUE* ClearValue, ResourceHolder** ppOutResource, const char* Name)
	{
		//CreateCommitResource
		COMPtr<ID3D12Resource> pResource;
		D3D12_HEAP_FLAGS HeapFlags = bHeapNotZeroedSupported ? D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;
		if (BufDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
		{
			HeapFlags |= D3D12_HEAP_FLAG_SHARED;
		}

		const HRESULT hr = d3d_device->CreateCommittedResource(&HeapProps, HeapFlags, &BufDesc, InInitialState, nullptr, COMPtr_RefParam(pResource, IID_ID3D12Resource));

		if (SUCCEEDED(CheckHResult(hr)))
		{
			// Set the output pointer
			*ppOutResource = new ResourceHolder(pResource, InDefaultState, BufDesc, HeapProps.Type);

			// Set a default name (can override later).
			(*ppOutResource)->SetName(Name);
		}

		return hr;
	}

	HRESULT D3D12::Device::CreatePlacedResource(const D3D12_RESOURCE_DESC& InDesc, HeapHolder* BackingHeap, uint64 HeapOffset, D3D12_RESOURCE_STATES InInitialState, ResourceStateMode InResourceStateMode, D3D12_RESOURCE_STATES InDefaultState, const D3D12_CLEAR_VALUE* ClearValue, ResourceHolder** ppOutResource, const char* Name)
	{
		if (!ppOutResource)
		{
			return E_POINTER;
		}

		ID3D12Heap* Heap = BackingHeap->GetHeap();

		COMPtr<ID3D12Resource> pResource;
		const HRESULT hr = d3d_device->CreatePlacedResource(Heap, HeapOffset, &InDesc, InInitialState, ClearValue, COMPtr_RefParam(pResource, IID_ID3D12Resource));

		if (SUCCEEDED(CheckHResult(hr)))
		{
			auto Device = BackingHeap->GetParentDevice();
			const D3D12_HEAP_DESC HeapDesc = Heap->GetDesc();

			// Set the output pointer
			*ppOutResource = new ResourceHolder(
				pResource,
				InInitialState,
				InDesc,
				BackingHeap,
				HeapDesc.Properties.Type);

			// Set a default name (can override later).
			(*ppOutResource)->SetName(Name);
		}

		return hr;
	}
}
