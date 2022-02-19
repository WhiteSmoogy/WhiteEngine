#pragma once

#include "NodeDevice.h"

namespace platform_ex::Windows::D3D12
{
	class ResourceAllocator;

	class ResourceLocation :public DeviceChild,public white::noncopyable
	{
	public:
		enum LocationType
		{
			Undefined,
			SubAllocation,
			FastAllocation,
		};

		ResourceLocation(NodeDevice* Parent);
		~ResourceLocation();

		void SetResource(ResourceHolder* Value);
		inline void SetType(LocationType Value) { Type = Value; }
		inline void SetMappedBaseAddress(void* Value) { MappedBaseAddress = Value; }
		inline void SetGPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS Value) { GPUVirtualAddress = Value; }
		inline void SetOffsetFromBaseOfResource(uint64 Value) { OffsetFromBaseOfResource = Value; }
		inline void SetSize(uint64 Value) { Size = Value; }
		inline void SetAllocator(ResourceAllocator* Value) { Allocator = Value; }

		inline ResourceHolder* GetResource() const { return UnderlyingResource; }
		inline void* GetMappedBaseAddress() const { return MappedBaseAddress; }
		inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return GPUVirtualAddress; }
		inline uint64 GetOffsetFromBaseOfResource() const { return OffsetFromBaseOfResource; }
		inline uint64 GetSize() const { return Size; }
		inline ResourceAllocator* GetAllocator() const {return Allocator; }


		void AsFastAllocation(ResourceHolder* Resource, uint32 BufferSize, D3D12_GPU_VIRTUAL_ADDRESS GPUBase, void* CPUBase, uint64 ResourceOffsetBase, uint64 Offset, bool bMultiFrame = false)
		{
			SetType(FastAllocation);
			SetResource(Resource);
			SetSize(BufferSize);
			SetOffsetFromBaseOfResource(ResourceOffsetBase + Offset);

			if (CPUBase != nullptr)
			{
				SetMappedBaseAddress((uint8*)CPUBase + Offset);
			}
			SetGPUVirtualAddress(GPUBase + Offset);
		}

		void Clear();
	private:
		void ClearResource();

		LocationType Type;

		ResourceHolder* UnderlyingResource;

		union
		{
			ResourceAllocator* Allocator;
		};

		void* MappedBaseAddress;
		D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
		uint64 OffsetFromBaseOfResource;

		// The size the application asked for
		uint64 Size;
	};

	class ResourceAllocator :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		ResourceAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes)
			:DeviceChild(InParentDevice),MultiNodeGPUObject(InParentDevice->GetGPUMask(),VisibleNodes)
		{}

		virtual void Deallocate(ResourceLocation& ResourceLocation);
	};

	class FastConstantPageAllocator :public ResourceAllocator
	{
	public:
		FastConstantPageAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes)
			:ResourceAllocator(InParentDevice, VisibleNodes)
		{}

		bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

		void CleanUpAllocations(uint64 InFrameLag);
	private:
		using ResourceAllocator::ResourceAllocator;

		class ConstantAllocator :public ResourceAllocator{
		public:
			ConstantAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, uint32 InBlockSize)
				:ResourceAllocator(InParentDevice,VisibleNodes),BlockSize(InBlockSize), TotalSizeUsed(0),BackingResource(nullptr), DelayCreated(false)
				, RetireFrameFence(-1)
			{}

			~ConstantAllocator();

			bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

			void Deallocate(ResourceLocation & ResourceLocation);

			uint64 GetLastUsedFrameFence() const { return RetireFrameFence; }
		private:
			void CreateBackingResource();

			const uint32 BlockSize;

			uint32 TotalSizeUsed;

			ResourceHolder* BackingResource;

			uint64 RetireFrameFence;

			bool DelayCreated;
		};

		ConstantAllocator* CreateNewAllocator(uint32 InMinSizeInBytes);

		std::vector<ConstantAllocator*> Allocators;
	};

	// This is designed for allocation of scratch memory such as temporary staging buffers
	// or shadow buffers for dynamic resources.
	class UploadHeapAllocator : public AdapterChild, public DeviceChild, public MultiNodeGPUObject
	{
	public:
		UploadHeapAllocator(D3D12Adapter* InParent, NodeDevice* InParentDevice, const std::string& InName);

		void Destroy();

		void* AllocFastConstantAllocationPage(uint32 InSize, uint32 InAlignment,ResourceLocation& ResourceLocation);

		void CleanUpAllocations(uint64 InFrameLag);

	private:
		// Seperate allocator used for the fast constant allocator pages which get always freed within the same frame by default
		// (different allocator to avoid fragmentation with the other pools - always the same size allocations)
		FastConstantPageAllocator FastConstantAllocator;
	};

	class FastConstantAllocator :public DeviceChild,public MultiNodeGPUObject
	{
	public:
		FastConstantAllocator(NodeDevice* Parent, GPUMaskType InGpuMask);

		void* Allocate(uint32 Bytes, ResourceLocation& OutLocation);
	private:
		ResourceLocation UnderlyingResource;

		uint32 Offset;
		uint32 PageSize;
	};
}