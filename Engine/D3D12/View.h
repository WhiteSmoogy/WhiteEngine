#pragma once

#include "d3d12_dxgi.h"
#include "Common.h"
#include "Utility.h"
#include "ResourceHolder.h"
#include "RenderInterface/IGPUResourceView.h"
#include "Descriptors.h"
#include "SamplerState.h"
#include "magic_enum.hpp"

namespace platform_ex::Windows::D3D12 {
	using platform::Render::SampleDesc;

	template<typename T>
	using Range = platform::Render::Range<T>;

	class NodeDevice;
	class GraphicsBuffer;

	enum ViewSubresourceSubsetFlags
	{
		ViewSubresourceSubsetFlags_None = 0x0,
		ViewSubresourceSubsetFlags_DepthOnlyDsv = 0x1,
		ViewSubresourceSubsetFlags_StencilOnlyDsv = 0x2,
		ViewSubresourceSubsetFlags_DepthAndStencilDsv = (ViewSubresourceSubsetFlags_DepthOnlyDsv | ViewSubresourceSubsetFlags_StencilOnlyDsv),
	};

	struct ViewRange
	{
		ViewRange() = default;

		ViewRange(const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc)
		{
			uint32 FirstPlane = GetPlaneSliceFromViewFormat(Desc.Format, Desc.Format);
			uint32 NumPlanes = GetPlaneCount(Desc.Format);
			Plane = { FirstPlane, NumPlanes - FirstPlane };

			switch (Desc.ViewDimension)
			{
			default: wassume(0 && "Corrupt Resource Type on Shader Resource View"); break;
			case D3D12_SRV_DIMENSION_UNKNOWN:
				Mip = { 0, 0 };
				Plane = { 0, 0 };
				Array = { 0, 0 };
				break;

			case (D3D12_SRV_DIMENSION_BUFFER):
				Mip = { 0, 1 };
				Array = { 0, 1 };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURE1D):
				Mip = { Desc.Texture1D.MostDetailedMip, Desc.Texture1D.MipLevels };
				Array = { 0 , 1 };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURE1DARRAY):
				Array = { Desc.Texture1DArray.FirstArraySlice , Desc.Texture1DArray.ArraySize };
				Mip = { Desc.Texture1DArray.MostDetailedMip, Desc.Texture1DArray.MipLevels };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURE2D):
				Mip = { Desc.Texture2D.MostDetailedMip, Desc.Texture2D.MipLevels };
				Plane = { Desc.Texture2D.PlaneSlice, 1 };
				Array = { 0, 1 };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURE2DARRAY):
				Mip = { Desc.Texture2DArray.MostDetailedMip, Desc.Texture2DArray.MipLevels };
				Plane = { Desc.Texture2DArray.PlaneSlice, 1 };
				Array = { Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.ArraySize };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURE2DMS):
				Mip = { 0, 1 };
				Array = { 0, 1 };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY):
				Mip = { 0, 1 };
				Array = { Desc.Texture2DMSArray.FirstArraySlice, Desc.Texture2DMSArray.ArraySize };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURE3D):
				Mip = { Desc.Texture3D.MostDetailedMip, Desc.Texture3D.MipLevels };
				Array = { 0, 1 };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURECUBE):
				Mip = { Desc.TextureCube.MostDetailedMip, Desc.TextureCube.MipLevels };
				Array = { 0, 1 };
				break;

			case (D3D12_SRV_DIMENSION_TEXTURECUBEARRAY):
				Mip = { Desc.TextureCubeArray.MostDetailedMip, Desc.TextureCubeArray.MipLevels };
				Array = { Desc.TextureCubeArray.First2DArrayFace, Desc.TextureCubeArray.NumCubes * 6 };
				break;
			case (D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE):
				Mip = { 0, 1 };
				Array = { 0, 1 };
				break;
			}
		}

		ViewRange(const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc)
		{
			uint32 FirstPlane = GetPlaneSliceFromViewFormat(Desc.Format, Desc.Format);
			uint32 NumPlanes = GetPlaneCount(Desc.Format);
			Plane = { FirstPlane, NumPlanes - FirstPlane };

			switch (Desc.ViewDimension)
			{
			default:wassume(0 && "Corrupt Resource Type on Unordered Access View");; return; // not implemented

			case D3D12_UAV_DIMENSION_UNKNOWN:
				Mip = { 0, 0 };
				Plane = { 0, 0 };
				Array = { 0, 0 };
				break;

			case D3D12_UAV_DIMENSION_BUFFER:
				Mip = { 0, 1 };
				Array = { 0, 1 };
				break;

			case D3D12_UAV_DIMENSION_TEXTURE2D:
				Mip = { Desc.Texture2D.MipSlice  , 1 };
				Plane = { Desc.Texture2D.PlaneSlice, 1 };
				Array = { 0, 1 };
				break;

			case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
				Mip = { Desc.Texture2DArray.MipSlice  , 1 };
				Plane = { Desc.Texture2DArray.PlaneSlice, 1 };
				Array = { Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.ArraySize };
				break;

			case D3D12_UAV_DIMENSION_TEXTURE3D:
				Mip = { Desc.Texture3D.MipSlice, 1 };
				Array = { 0, 1 };
				break;
			}
		}

		ViewRange(const D3D12_RENDER_TARGET_VIEW_DESC& Desc)
		{
			uint32 FirstPlane = GetPlaneSliceFromViewFormat(Desc.Format, Desc.Format);
			uint32 NumPlanes = GetPlaneCount(Desc.Format);
			Plane = { FirstPlane, NumPlanes - FirstPlane };

			switch (Desc.ViewDimension)
			{
			default: wassume(0 && "Corrupt Resource Type on Render Target View"); break;

			case D3D12_RTV_DIMENSION_UNKNOWN:
				Mip = { 0, 0 };
				Plane = { 0, 0 };
				Array = { 0, 0 };
				break;

			case D3D12_RTV_DIMENSION_TEXTURE2D:
				Mip = { Desc.Texture2D.MipSlice  , 1 };
				Plane = { Desc.Texture2D.PlaneSlice, 1 };
				Array = { 0, 1 };
				break;

			case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
				Mip = { Desc.Texture2DArray.MipSlice  , 1 };
				Plane = { Desc.Texture2DArray.PlaneSlice, 1 };
				Array = { Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.ArraySize };
				break;

			case D3D12_RTV_DIMENSION_TEXTURE2DMS:
				Mip = { 0, 1 };
				Array = { 0, 1 };
				break;

			case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
				Mip = { 0, 1 };
				Array = { Desc.Texture2DMSArray.FirstArraySlice, Desc.Texture2DMSArray.ArraySize };
				break;

			case D3D12_RTV_DIMENSION_TEXTURE3D:
				Mip = { Desc.Texture3D.MipSlice, 1 };
				Array = { 0, 1 };
				break;
			}
		}

		ViewRange(const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc)
		{
			uint32 FirstPlane = GetPlaneSliceFromViewFormat(Desc.Format, Desc.Format);
			uint32 NumPlanes = GetPlaneCount(Desc.Format);
			Plane = { FirstPlane, NumPlanes - FirstPlane };

			switch (Desc.ViewDimension)
			{
			default: wassume(0 && "Corrupt Resource Type on Depth Stencil View"); break;

			case D3D12_DSV_DIMENSION_UNKNOWN:
				Mip = { 0, 0 };
				Plane = { 0, 0 };
				Array = { 0, 0 };
				break;

			case D3D12_DSV_DIMENSION_TEXTURE2D:
				Mip = { Desc.Texture2D.MipSlice, 1 };
				Array = { 0, 1 };
				break;

			case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
				Mip = { Desc.Texture2DArray.MipSlice, 1 };
				Array = { Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.ArraySize };
				break;

			case D3D12_DSV_DIMENSION_TEXTURE2DMS:
				Mip = { 0, 1 };
				Array = { 0, 1 };
				break;

			case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
				Mip = { 0, 1 };
				Array = { Desc.Texture2DMSArray.FirstArraySlice, Desc.Texture2DMSArray.ArraySize };
				break;
			}
		}

		bool DoesNotOverlap(const ViewRange& Other) const
		{
			return Mip.ExclusiveLast() <= Other.Mip.First || Other.Mip.ExclusiveLast() <= Mip.First
				|| Array.ExclusiveLast() <= Other.Array.First || Other.Array.ExclusiveLast() <= Array.First
				|| Plane.ExclusiveLast() <= Other.Plane.First || Other.Plane.ExclusiveLast() <= Plane.First;
		}

		Range<uint16> Array; 
		Range<uint8> Plane;
		Range<uint8> Mip; 
	};

	struct ResourceLayout
	{
		ResourceLayout() = default;

		ResourceLayout(const D3D12_RESOURCE_DESC& ResourceDesc)
			:NumArraySlices(ResourceDesc.DepthOrArraySize)
			, NumPlanes(GetPlaneCount(ResourceDesc.Format))
			, NumMips(ResourceDesc.MipLevels)
		{}

		uint16 NumArraySlices = 0;
		uint8  NumPlanes = 0;
		uint8  NumMips = 0;
	};

	struct CViewSubset
	{
		CViewSubset() = default;

		CViewSubset(ResourceLayout const& Layout, ViewRange const& Range)
			: Layout(Layout)
			, Range(Range)
		{}

		class CViewSubsetIterator;

		CViewSubsetIterator begin() const;
		CViewSubsetIterator end() const;
		bool IsWholeResource() const
		{
			return
				Range.Mip.First == 0 && Range.Mip.Num == Layout.NumMips &&
				Range.Array.First == 0 && Range.Array.Num == Layout.NumArraySlices &&
				Range.Plane.First == 0 && Range.Plane.Num == Layout.NumPlanes;
		}

		bool HasPlane(uint32 PlaneIndex) const
		{
			return Range.Plane.IsInRange(PlaneIndex);
		}

		CViewSubset SelectPlane(uint32 PlaneIndex) const
		{
			wassume(PlaneIndex >= Range.Plane.First && PlaneIndex < Range.Plane.ExclusiveLast());

			CViewSubset Copy{ *this };
			Copy.Range.Plane = { PlaneIndex, 1u };

			return Copy;
		}

		ResourceLayout Layout;
		ViewRange      Range;
	};

	class CViewSubset::CViewSubsetIterator
	{
	public:
		inline CViewSubsetIterator(CViewSubset const& ViewSubset, uint8 MipSlice, uint16 ArraySlice, uint8 PlaneSlice)
			: MipMax(ViewSubset.Range.Mip.Num)
			, ArrayMax(ViewSubset.Range.Array.Num)
			, ArrayStride(ViewSubset.Layout.NumMips)
			, PlaneStride(ViewSubset.Layout.NumMips* (ViewSubset.Layout.NumArraySlices - ViewSubset.Range.Array.Num))
		{
			MipRangeStart = CalcSubresource(
				MipSlice,
				ArraySlice,
				PlaneSlice,
				ViewSubset.Layout.NumMips,
				ViewSubset.Layout.NumArraySlices
			);
		}

		inline CViewSubset::CViewSubsetIterator& operator++()
		{
			if (++MipOffset == MipMax)
			{
				// Move to next array slice
				MipOffset = 0;
				MipRangeStart += ArrayStride;

				if (++ArrayOffset == ArrayMax)
				{
					// Move to next plane slice
					ArrayOffset = 0;
					MipRangeStart += PlaneStride;
				}
			}

			return *this;
		}

		uint32 operator * () const { return MipRangeStart + MipOffset; }

		bool operator == (CViewSubsetIterator const& RHS) const { return *(*this) == *RHS; }
		bool operator != (CViewSubsetIterator const& RHS) const { return !(*this == RHS); }
	private:
		// Constants
		uint32 const MipMax;
		uint32 const ArrayMax;
		uint32 const ArrayStride;
		uint32 const PlaneStride;

		// Counters
		uint32 MipRangeStart;
		uint32 MipOffset = 0;
		uint32 ArrayOffset = 0;
	};

	inline CViewSubset::CViewSubsetIterator CViewSubset::begin() const
	{
		return CViewSubsetIterator(*this, Range.Mip.First, Range.Array.First, Range.Plane.First);
	}

	inline CViewSubset::CViewSubsetIterator CViewSubset::end() const
	{
		return CViewSubsetIterator(*this, Range.Mip.First, Range.Array.First, Range.Plane.ExclusiveLast());
	}

	template<typename TDesc>
	constexpr bool TIsD3D12SRVDescriptorHandleValue = false;

	template<>
	constexpr bool TIsD3D12SRVDescriptorHandleValue<D3D12_SHADER_RESOURCE_VIEW_DESC> = true;

	struct D3DDefaultViews
	{
		OfflineDescriptor NullSRV;
		OfflineDescriptor NullRTV;
		OfflineDescriptor NullUAV;
		OfflineDescriptor NullCBV;
		OfflineDescriptor NullDSV;

		SamplerState* DefaultSampler;
	};

	class D3DView : DeviceChild
	{
		typedef OfflineDescriptor D3DDefaultViews::* NullDescPtr;

	public:
		inline OfflineDescriptor	GetOfflineCpuHandle()					const { return OfflineHandle; }
		inline ResourceHolder* GetResource()				const { return Resource; }
		inline ResourceLocation* GetResourceLocation()		const { return Location; }
		const CViewSubset& GetViewSubset()	const {
			return ViewSubset;
		}

	protected:
		OfflineDescriptor OfflineHandle;

		ViewSubresourceSubsetFlags Flags;
		BaseShaderResource* ShaderResource;
		ResourceLocation* Location;
		ResourceHolder* Resource;
		CViewSubset ViewSubset;

		DescriptorHeapType HeapType;
	protected:
		D3DView(NodeDevice* InDevice, DescriptorHeapType InHeapType);
		virtual ~D3DView();

		virtual void UpdateDescriptor() = 0;

		void CreateView(BaseShaderResource* InResource, NullDescPtr NullDescriptor);
		void CreateView(ResourceLocation* InResource, NullDescPtr NullDescriptor);
	};

	template <typename TParent, typename TDesc>
	class TView :public D3DView
	{
	protected:
		TDesc Desc;

		explicit TView(NodeDevice* InParent, DescriptorHeapType InHeapType)
			: D3DView(InParent, InHeapType)
		{}
		
		template<typename TResource>
		void CreateView(const TDesc& InD3DViewDesc, TResource InResource)
		{
			Desc = InD3DViewDesc;
			ViewSubset.Range = InD3DViewDesc;
			D3DView::CreateView(InResource, TParent::Null);
		}

	public:
		inline const TDesc& GetDesc()					const { return Desc; }
	};

	class RenderTargetView : public TView<RenderTargetView, D3D12_RENDER_TARGET_VIEW_DESC>
	{
	public:
		static constexpr OfflineDescriptor D3DDefaultViews::* Null{ &D3DDefaultViews::NullRTV };

		RenderTargetView(NodeDevice* InParent);

		using TView::CreateView;

	private:
		void UpdateDescriptor() override;
	};

	class DepthStencilView :public TView<DepthStencilView, D3D12_DEPTH_STENCIL_VIEW_DESC>
	{
	public:
		static constexpr OfflineDescriptor D3DDefaultViews::* Null{ &D3DDefaultViews::NullDSV };

		DepthStencilView(NodeDevice* InParent);

		using TView::CreateView;

		bool HasDepth() const { return GetViewSubset().HasPlane(0); }
		bool HasStencil() const { return GetViewSubset().HasPlane(1); }

		CViewSubset GetDepthOnlySubset() const { return GetViewSubset().SelectPlane(0); }
		CViewSubset GetStencilOnlySubset() const { return GetViewSubset().SelectPlane(1); }

	private:
		void UpdateDescriptor() override;
	};

	using namespace magic_enum::bitwise_operators;

	class ShaderResourceView : public platform::Render::ShaderResourceView, public TView<ShaderResourceView, D3D12_SHADER_RESOURCE_VIEW_DESC>
	{
	public:
		static constexpr OfflineDescriptor D3DDefaultViews::* Null{ &D3DDefaultViews::NullSRV };

		enum class EFlags : uint8
		{
			None = 0,
			SkipFastClearFinalize = 1 << 0,
		};

		ShaderResourceView(NodeDevice* InDevice);

		template<typename TResource>
		void CreateView(TResource InResource, D3D12_SHADER_RESOURCE_VIEW_DESC const& InD3DViewDesc, EFlags InFlags);

		bool GetSkipFastClearFinalize() const { return white::has_anyflags(Flags, EFlags::SkipFastClearFinalize); }
		void UpdateMinLODClamp(float MinLODClamp);

	protected:
		void UpdateDescriptor() override;

		// Required for resource renaming
		uint64 OffsetInBytes = 0;
		uint32 StrideInBytes = 0;

		EFlags Flags = EFlags::None;
	};

	class UnorderedAccessView : public platform::Render::UnorderedAccessView, public TView <UnorderedAccessView, D3D12_UNORDERED_ACCESS_VIEW_DESC>
	{
	public:
		static constexpr OfflineDescriptor D3DDefaultViews::* Null{ &D3DDefaultViews::NullUAV };

		enum class EFlags : uint8
		{
			None = 0,
			NeedsCounter = 1 << 0
		};

		UnorderedAccessView(NodeDevice* InDevice);
		template<typename TResource>
		void CreateView(TResource InResource, D3D12_UNORDERED_ACCESS_VIEW_DESC const& InD3DViewDesc, EFlags InFlags);

		ResourceHolder* GetCounterResource() const
		{
			return CounterResource.Get();
		}

	protected:
		virtual void UpdateDescriptor() override;

		COMPtr<ResourceHolder> CounterResource;

		// Required for resource renaming
		uint64 OffsetInBytes = 0;
		uint32 StrideInBytes = 0;
	};

}