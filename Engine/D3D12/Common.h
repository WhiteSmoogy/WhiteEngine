#pragma once

/*

|-[Render::Context]--
					 |-[Adapter]-- (LDA)
					 |			|
					 |			|- [Device]
					 |			|
					 |			|- [Device]
					 |
					 |-[Adapter]--
					 			|
					 			|- [Device]--
					 						|
					 						|-[CommandContext]
					 						|
					 						|-[CommandContext]--
					 										  |-[StateCache]
Under this scheme an NodeDevice represents 1 node belonging to 1 physical adapter.

This structure allows a single Render::Context to control several different hardware setups. Some example arrangements:
	- Single-GPU systems (the common case)
	- Multi-GPU systems i.e. LDA (Crossfire/SLI)
	- Asymmetric Multi-GPU systems i.e. Discrete/Integrated GPU cooperation												
*/

#include <WBase/cassert.h>
#include <WBase/winttype.hpp>

namespace platform_ex::Windows::D3D12 {
	using namespace white::inttype;

	constexpr auto MAX_NUM_GPUNODES = 4;

	class Device;

	//TODO:different hardware setups
	//CommonCase Map
	class NodeDevice;

	using D3D12Adapter = Device;

	struct GPUMaskType
	{
	private:
		uint32 GPUMask;

		constexpr explicit GPUMaskType(uint32 InGPUMask)
			:GPUMask(InGPUMask)
		{
		}
	public:
		constexpr GPUMaskType() :GPUMaskType(GPU0())
		{}

		static constexpr GPUMaskType FromIndex(uint32 GPUIndex) { return GPUMaskType(1 << GPUIndex); }

		constexpr uint32 ToIndex() const
		{
			return std::countr_zero(GPUMask);
		}

		static constexpr GPUMaskType GPU0()
		{
			return GPUMaskType(1);
		}

		static constexpr GPUMaskType AllGPU()
		{
			constexpr auto NumExplicitGPUs = 1;
			return GPUMaskType((1<< NumExplicitGPUs) -1);
		}

		constexpr uint32 GetNative() const { return GPUMask; }

		constexpr bool operator==(const GPUMaskType&) const = default;
	};

	class DeviceChild
	{
	protected:
		NodeDevice* Parent;
	public:
		DeviceChild(NodeDevice* InParent = nullptr) : Parent(InParent) {}

		NodeDevice* GetParentDevice() const{ return Parent; }

		void SetParentDevice(NodeDevice* InParent)
		{
			wconstraint(Parent == nullptr);
			Parent = InParent;
		}
	};

	class AdapterChild
	{
	protected:
		D3D12Adapter* ParentAdapter;

	public:
		AdapterChild(D3D12Adapter* InParent = nullptr) : ParentAdapter(InParent) {}

		D3D12Adapter* GetParentAdapter() { return ParentAdapter; }

		// To be used with delayed setup
		void SetParentAdapter(D3D12Adapter* InParent)
		{
			ParentAdapter = InParent;
		}
	};

	class GPUObject
	{
	public:
		GPUObject(GPUMaskType InGPUMask, GPUMaskType InVisibiltyMask)
			: GPUMask(InGPUMask)
			, VisibilityMask(InVisibiltyMask)
		{
			// Note that node mask can't be null.
		}

		const GPUMaskType GetGPUMask() const { return GPUMask; }

		const GPUMaskType GetVisibilityMask() const { return VisibilityMask; }

	protected:
		GPUMaskType GPUMask;
		// Which GPUs have direct access to this object
		GPUMaskType VisibilityMask;
	};

	class SingleNodeGPUObject :public GPUObject
	{
	public:
		SingleNodeGPUObject(GPUMaskType GPUMask)
			:GPUObject(GPUMask,GPUMask)
			,GPUIndex(GPUMask.ToIndex())
		{}

		uint32 GetGPUIndex() const
		{
			return GPUIndex;
		}
	protected:
		uint32 GPUIndex;
	};

	class MultiNodeGPUObject :public GPUObject
	{
	public:
		MultiNodeGPUObject(GPUMaskType NodeMask, GPUMaskType VisibiltyMask)
			:GPUObject(NodeMask, VisibiltyMask)
		{}
	};

	NodeDevice* GetDefaultNodeDevice();
}