#pragma once

#include "../IDevice.h"
#include "DXGIAdapter.h"
#include "Texture.h"
#include "ShaderCompose.h"
#include "GraphicsBuffer.hpp"
#include "InputLayout.hpp"
#include "GraphicsPipelineState.h"
#include "Display.h"
#include "PipleState.h"
#include "Fence.h"
#include "RootSignature.h"
#include "D3DCommandList.h"
#include "ConstantBuffer.h"

namespace platform::Render::Effect {
	class Effect;
	class CopyEffect;
}

namespace platform_ex::Windows::D3D12 {
	class NodeDevice;

	namespace  Effect = platform::Render::Effect;
	using namespace platform::Render::IFormat;
	using platform::Render::SampleDesc;
	using platform::Render::ElementInitData;
	namespace Buffer = platform::Render::Buffer;

	struct ResourceCreateInfo
	{
		unsigned int GPUIndex = 0;

		bool WithoutNativeResource = true;
		const char* DebugName = "unknown";
		const void* ResouceData = nullptr;
	};

	class IResourceAllocator;

	ResourceCreateInfo FillResourceCreateInfo(std::optional<void const*> init_data, const char* DebugName);
		
	// Represents a set of linked D3D12 device nodes (LDA i.e 1 or more identical GPUs). In most cases there will be only 1 node, however if the system supports
	// SLI/Crossfire and the app enables it an Adapter will have 2 or more nodes. This class will own anything that can be shared
	// across LDA including: System Pool Memory,.Pipeline State Objects, Root Signatures etc.
	class Device final :public platform::Render::Device {
	public:
		Device(DXGI::Adapter& InAdapter);

		ID3D12Device* operator->() wnoexcept;

		Texture1D* CreateTexture(uint16 width, uint8 num_mipmaps, uint8 array_size,
			EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const*>  init_data = nullptr) override;

		Texture2D* CreateTexture(uint16 width, uint16 height, uint8 num_mipmaps, uint8 array_size,
			EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const*>  init_data = nullptr) override;

		Texture3D* CreateTexture(const platform::Render::Texture3DInitializer& Initializer, std::optional<ElementInitData const*>  init_data = nullptr) override;

		TextureCube* CreateTextureCube(uint16 size, uint8 num_mipmaps, uint8 array_size,
			EFormat format, uint32 access, SampleDesc sample_info, std::optional<ElementInitData const*>  init_data = nullptr) override;

		platform::Render::Caps& GetCaps() override;

		bool IsHeapNotZeroedSupported() const
		{
			return bHeapNotZeroedSupported;
		}

		ShaderCompose* CreateShaderCompose(std::unordered_map<platform::Render::ShaderType, const asset::ShaderBlobAsset*> pShaderBlob, platform::Render::Effect::Effect* pEffect) override;

		//\brief D3D12 Buffer 创建时没有BIND_FLAG
		GraphicsBuffer* CreateBuffer(platform::Render::CommandList* Cmdlist,Buffer::Usage usage, white::uint32 access,uint32 size, uint32 stride, DXGI_FORMAT format, ResourceCreateInfo& CreateInfo);

	 GraphicsBuffer* CreateBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, uint32 stride, std::optional<void const*>  init_data) override;


		template<ResourceStateMode Mode>
		GraphicsBuffer* CreateBuffer(platform::Render::CommandList* Cmdlist, const D3D12_RESOURCE_DESC& InDesc,
			Buffer::Usage usage, white::uint32 access, 
			uint32 Alignment,uint32 Stride, uint64 Size, ResourceCreateInfo& CreateInfo,
			IResourceAllocator* ResourceAllocator);

		ConstantBuffer* CreateConstantBuffer(Buffer::Usage usage, uint32 size_in_byte,const void*  init_data) override;
		GraphicsBuffer* CreateVertexBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, EFormat format, std::optional<void const*>  init_data = nullptr) override;
		GraphicsBuffer* CreateIndexBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, EFormat format, std::optional<void const*>  init_data = nullptr) override;

		platform::Render::HardwareShader* CreateShader(const white::span<const uint8>& Code,platform::Render::ShaderType Type);

		platform::Render::HardwareShader* CreateVertexShader(const white::span<const uint8>& Code)override;
		platform::Render::HardwareShader* CreatePixelShader(const white::span<const uint8>& Code) override;
		platform::Render::HardwareShader* CreateGeometryShader(const white::span<const uint8>& Code) override;
		platform::Render::HardwareShader* CreateComputeShader(const white::span<const uint8>& Code)  override;

		white::observer_ptr<RootSignature> CreateRootSignature(const QuantizedBoundShaderState& QBSS);

		PipleState* CreatePipleState(const platform::Render::PipleState& state) override;

		InputLayout* CreateInputLayout() override;

		GraphicsPipelineState* CreateGraphicsPipelineState(const platform::Render::GraphicsPipelineStateInitializer& initializer) override;

		ComputePipelineState* CreateComputePipelineState(platform::Render::ComputeHWShader* ComputeShader);

		UnorderedAccessView* CreateUnorderedAccessView(platform::Render::Texture2D* InTexture) override;

		platform::Render::Effect::CopyEffect& BiltEffect();

		platform::Render::InputLayout& PostProcessLayout();

		white::observer_ptr<ID3D12DescriptorHeap> CreateDynamicCBVSRVUAVDescriptorHeap(uint32_t num);

		white::observer_ptr<ID3D12PipelineState> CreateRenderPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC&);


		ID3D12Device* GetDevice() const
		{
			return d3d_device.Get();
		}

		ID3D12Device5* GetRayTracingDevice()
		{
			return raytracing_device.Get();
		}

		NodeDevice* GetNodeDevice(uint32 Index) const
		{
			return Devices[Index];
		}

		UploadHeapAllocator& GetUploadHeapAllocator(uint32 GPUIndex);

		void InitializeRayTracing();

		ManualFence& GetFrameFence() { return *frame_fence; }

		void EndFrame();

		FastConstantAllocator& GetTransientConstantBufferAllocator();

		HRESULT CreateBuffer(D3D12_HEAP_TYPE HeapType,
			GPUMaskType CreationNode,
			GPUMaskType VisibleNodes,
			uint64 HeapSize,
			ResourceHolder** ppOutResource,
			const char* Name,
			D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE);

		HRESULT CreateBuffer(const D3D12_HEAP_PROPERTIES& HeapProps,
			GPUMaskType CreationNode,
			D3D12_RESOURCE_STATES InitialState,
			D3D12_RESOURCE_STATES InDefaultState,
			uint64 HeapSize,
			ResourceHolder** ppOutResource,
			const char* Name,
			D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE);

		template<ResourceStateMode Mode>
		void AllocateBuffer(NodeDevice* Device,
			const D3D12_RESOURCE_DESC& InDesc,
			uint32 Size,
			uint32 InUsage,
			uint32 InAccess,
			D3D12_RESOURCE_STATES InCreateState,
			ResourceCreateInfo& CreateInfo,
			uint32 Alignment,
			GraphicsBuffer* Buffer,
			ResourceLocation& ResourceLocation,
			IResourceAllocator* ResourceAllocator);

		HRESULT CreateCommittedResource(const D3D12_RESOURCE_DESC& InDesc, GPUMaskType CreationNode, const D3D12_HEAP_PROPERTIES& HeapProps, D3D12_RESOURCE_STATES InInitialState,
			D3D12_RESOURCE_STATES InDefaultState, const D3D12_CLEAR_VALUE* ClearValue, ResourceHolder** ppOutResource, const char* Name);

		HRESULT CreatePlacedResource(const D3D12_RESOURCE_DESC& InDesc, HeapHolder* BackingHeap, uint64 HeapOffset, D3D12_RESOURCE_STATES InInitialState, ResourceStateMode InResourceStateMode,
			D3D12_RESOURCE_STATES InDefaultState, const D3D12_CLEAR_VALUE* ClearValue, ResourceHolder** ppOutResource, const char* Name);
	public:
		friend class Context;
		friend class RayContext;
		friend class RayDevice;

		enum CommandType {
			Command_Resource = 1,
			CommandTypeCount
		};

		//@{
		//\brief 使用者可以修改这些值满足特定需求
		wconstexpr static UINT const NUM_MAX_RENDER_TARGET_VIEWS = 1024 + Display::NUM_BACK_BUFFERS;
		wconstexpr static UINT const NUM_MAX_DEPTH_STENCIL_VIEWS = 128;
		wconstexpr static UINT const NUM_MAX_CBV_SRV_UAVS = 4 * 1024;

		//@}
	private:
		void FillCaps();

		void DeviceEx(ID3D12Device* device, ID3D12CommandQueue* cmd_queue, D3D_FEATURE_LEVEL feature_level);


		void CheckFeatureSupport(ID3D12Device* device);

		struct ResidencyPoolEntry
		{
			CLSyncPoint SyncPoint;
			std::vector<COMPtr<ID3D12Resource>> recycle_after_sync_residency_buffs;
			std::vector<std::pair<COMPtr<ID3D12Resource>, uint32_t>> recycle_after_sync_upload_buffs;
			std::vector<std::pair<COMPtr<ID3D12Resource>, uint32_t>> recycle_after_sync_readback_buffs;
		};

		ResidencyPoolEntry ResidencyPool;
		std::queue< ResidencyPoolEntry> ReclaimPool;
	private:
		DXGI::Adapter& adapter;

		//@{
		//\brief base object for swapchain
		COMPtr<ID3D12Device> d3d_device;
		COMPtr<ID3D12Device5> raytracing_device;
		//@}

		COMPtr<ID3D12DebugDevice> d3d_debug_device;

		D3D_FEATURE_LEVEL d3d_feature_level;

		//@{
		//\brief object for create object
		array<COMPtr<ID3D12CommandAllocator>, CommandTypeCount> d3d_cmd_allocators;
		//COMPtr<ID3D12CommandQueue> d3d_cmd_compute_quque;
		//COMPtr<ID3D12CommandQueue> d3d_cmd_copy_quque;

		array<COMPtr<ID3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> d3d_desc_heaps;
		array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> d3d_desc_incres_sizes;

		//@{
		//\brief cache Device Object for performance
		std::unordered_map<size_t, COMPtr<ID3D12DescriptorHeap>> cbv_srv_uav_heaps;
		//@}

		//@}

		platform::Render::Caps d3d_caps;
		bool bHeapNotZeroedSupported;

		std::unique_ptr<platform::Render::Effect::CopyEffect> bilt_effect;
		std::unique_ptr<InputLayout> postprocess_layout;

		std::unique_ptr<RootSignatureMap> root_signatures;

		std::unordered_map<size_t, COMPtr<ID3D12PipelineState>> graphics_psos;

		std::multimap<uint32_t, COMPtr<ID3D12Resource>> upload_resources;
		std::multimap<uint32_t, COMPtr<ID3D12Resource>> readback_resources;

		array<std::unique_ptr<Fence>, Device::CommandTypeCount> fences;

		ManualFence* frame_fence;

		//feature support
		D3D12_RESOURCE_BINDING_TIER ResourceBindingTier;
		D3D12_RESOURCE_HEAP_TIER ResourceHeapTier;

		D3D_ROOT_SIGNATURE_VERSION RootSignatureVersion;

		UploadHeapAllocator* UploadHeapAllocators[1];
		NodeDevice* Devices[1];

		D3DPipelineStateCache PipelineStateCache;
	public:
		D3D12_RESOURCE_BINDING_TIER GetResourceBindingTier() const
		{
			return ResourceBindingTier;
		}

		D3D_ROOT_SIGNATURE_VERSION GetRootSignatureVersion() const
		{
			return RootSignatureVersion;
		}
	};


}
