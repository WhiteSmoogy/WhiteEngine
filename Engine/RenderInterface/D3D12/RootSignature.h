#pragma once

#ifndef WE_RENDER_D3D12_RootSignature_h
#define WE_RENDER_D3D12_RootSignature_h 1

#include "../Shader.h"
#include "d3d12_dxgi.h"
#include <WBase/wmemory.hpp>
#include <mutex>

namespace platform_ex::Windows::D3D12 {
	using namespace platform::Render::Shader;
	using platform::Render::ShaderBlob;
	using platform::Render::ShaderCodeResourceCounts;

	constexpr white::uint32 ShaderVisibilityCount = 6;

	enum class RootSignatureType
	{
		Raster,
		RayTracingGlobal,
		RayTracingLocal
	};


	struct QuantizedBoundShaderState
	{
		ShaderCodeResourceCounts RegisterCounts[ShaderVisibilityCount];
		RootSignatureType RootSignatureType = RootSignatureType::Raster;
		bool AllowIAInputLayout;
		bool AllowStreamOuput = false;

		inline bool operator==(const QuantizedBoundShaderState& rhs) const
		{
			return 0 == std::memcmp(this, &rhs, sizeof(rhs));
		}

		uint32 GetHashCode() const;

		static void InitShaderRegisterCounts(const D3D12_RESOURCE_BINDING_TIER& ResourceBindingTier, const ShaderCodeResourceCounts& Counts, ShaderCodeResourceCounts& Shader, bool bAllowUAVs = false);
	};

	class RootSignatureDesc
	{
	public:
		explicit RootSignatureDesc(const QuantizedBoundShaderState& QBSS, const D3D12_RESOURCE_BINDING_TIER ResourceBindingTier);

		inline const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& GetDesc() const { return RootDesc; }

		static constexpr uint32 MaxRootParameters = 32;	// Arbitrary max, increase as needed.
	private:

		uint32 RootParametersSize;	// The size of all root parameters in the root signature. Size in DWORDs, the limit is 64.
		CD3DX12_ROOT_PARAMETER1 TableSlots[MaxRootParameters];
		CD3DX12_DESCRIPTOR_RANGE1 DescriptorRanges[MaxRootParameters];
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootDesc;
	};

	// Root parameter keys grouped by visibility.
	enum RootParameterKeys
	{
		PS_SRVs,
		PS_CBVs,
		PS_RootCBVs,
		PS_Samplers,
		VS_SRVs,
		VS_CBVs,
		VS_RootCBVs,
		VS_Samplers,
		GS_SRVs,
		GS_CBVs,
		GS_RootCBVs,
		GS_Samplers,
		HS_SRVs,
		HS_CBVs,
		HS_RootCBVs,
		HS_Samplers,
		DS_SRVs,
		DS_CBVs,
		DS_RootCBVs,
		DS_Samplers,
		ALL_SRVs,
		ALL_CBVs,
		ALL_RootCBVs,
		ALL_Samplers,
		ALL_UAVs,
		RPK_RootParameterKeyCount,
	};

	class RootSignature
	{
	private:
		// Struct for all the useful info we want per shader stage.
		struct ShaderStage
		{
			ShaderStage()
				: MaxCBVCount(0u)
				, MaxSRVCount(0u)
				, MaxSamplerCount(0u)
				, MaxUAVCount(0u)
				, CBVRegisterMask(0u)
				, bVisible(false)
			{
			}

			uint8 MaxCBVCount;
			uint8 MaxSRVCount;
			uint8 MaxSamplerCount;
			uint8 MaxUAVCount;
			CBVSlotMask CBVRegisterMask;
			bool bVisible;
		};
	public:
		RootSignature()
		{}

		RootSignature(const ShaderBlob& blob)
		{
			Init(blob);
		}
		RootSignature(const QuantizedBoundShaderState& QBSS,ID3D12Device* pDevice)
		{
			Init(QBSS,pDevice);
		}

		void Init(const ShaderBlob& blob);
		void Init(const QuantizedBoundShaderState& QBSS,ID3D12Device* pDevice);
		void Init(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& InDesc, uint32 BindingSpace, ID3D12Device* pDevice);

		uint32 GetTotalRootSignatureSizeInBytes() const { return 4 * TotalRootSignatureSizeInDWORDs; }

		inline uint32 SamplerRDTBindSlot(ShaderType ShaderStage) const
		{
			switch (ShaderStage)
			{
			case ShaderType::VertexShader: return BindSlotMap[VS_Samplers];
			case ShaderType::PixelShader: return BindSlotMap[PS_Samplers];
			case ShaderType::GeometryShader: return BindSlotMap[GS_Samplers];
			case ShaderType::HullShader: return BindSlotMap[HS_Samplers];
			case ShaderType::DomainShader: return BindSlotMap[DS_Samplers];
			case ShaderType::ComputeShader: return BindSlotMap[ALL_Samplers];

			default: wconstraint(false);
				return UINT_MAX;
			}
		}

		inline uint32 SRVRDTBindSlot(ShaderType ShaderStage) const
		{
			switch (ShaderStage)
			{
			case ShaderType::VertexShader: return BindSlotMap[VS_SRVs];
			case ShaderType::PixelShader: return BindSlotMap[PS_SRVs];
			case ShaderType::GeometryShader: return BindSlotMap[GS_SRVs];
			case ShaderType::HullShader: return BindSlotMap[HS_SRVs];
			case ShaderType::DomainShader: return BindSlotMap[DS_SRVs];
			case ShaderType::ComputeShader: return BindSlotMap[ALL_SRVs];

			default: wconstraint(false);
				return UINT_MAX;
			}
		}

		inline uint32 CBVRDTBindSlot(ShaderType ShaderStage) const
		{
			switch (ShaderStage)
			{
			case ShaderType::VertexShader: return BindSlotMap[VS_CBVs];
			case ShaderType::PixelShader: return BindSlotMap[PS_CBVs];
			case ShaderType::GeometryShader: return BindSlotMap[GS_CBVs];
			case ShaderType::HullShader: return BindSlotMap[HS_CBVs];
			case ShaderType::DomainShader: return BindSlotMap[DS_CBVs];
			case ShaderType::ComputeShader: return BindSlotMap[ALL_CBVs];

			default: wconstraint(false);
				return UINT_MAX;
			}
		}

		inline uint32 CBVRDBaseBindSlot(ShaderType ShaderStage) const
		{
			switch (ShaderStage)
			{
			case ShaderType::VertexShader: return BindSlotMap[VS_RootCBVs];
			case ShaderType::PixelShader: return BindSlotMap[PS_RootCBVs];
			case ShaderType::GeometryShader: return BindSlotMap[GS_RootCBVs];
			case ShaderType::HullShader: return BindSlotMap[HS_RootCBVs];
			case ShaderType::DomainShader: return BindSlotMap[DS_RootCBVs];

			case ShaderType::NumStandardType:
			case ShaderType::ComputeShader: return BindSlotMap[ALL_RootCBVs];

			default: wconstraint(false);
				return UINT_MAX;
			}
		}

		inline uint32 CBVRDBindSlot(ShaderType ShaderStage, uint32 BufferIndex) const
		{
			// This code assumes that all Root CBVs for a particular stage are contiguous in the root signature (thus indexable by the buffer index).
			return CBVRDBaseBindSlot(ShaderStage) + BufferIndex;
		}

		inline uint32 UAVRDTBindSlot(ShaderType ShaderStage) const
		{
			wconstraint(ShaderStage == ShaderType::PixelShader || ShaderStage == ShaderType::ComputeShader);
			return BindSlotMap[ALL_UAVs];
		}

		inline uint32 MaxSamplerCount(uint32 ShaderStage) const { wconstraint(ShaderStage != ShaderType::NumStandardType); return Stage[ShaderStage].MaxSamplerCount; }
		inline uint32 MaxSRVCount(uint32 ShaderStage) const { wconstraint(ShaderStage != ShaderType::NumStandardType); return Stage[ShaderStage].MaxSRVCount; }
		inline uint32 MaxCBVCount(uint32 ShaderStage) const { wconstraint(ShaderStage != ShaderType::NumStandardType); return Stage[ShaderStage].MaxCBVCount; }
		inline uint32 MaxUAVCount(uint32 ShaderStage) const { wconstraint(ShaderStage != ShaderType::NumStandardType); return Stage[ShaderStage].MaxUAVCount; }
		inline CBVSlotMask CBVRegisterMask(uint32 ShaderStage) const { wconstraint(ShaderStage != ShaderType::NumStandardType); return Stage[ShaderStage].CBVRegisterMask; }

		ID3D12RootSignature* GetSignature() const
		{
			return Signature.Get();
		}
	private:
		void AnalyzeSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& Desc, uint32 BindingSpace);
		template<typename RootSignatureDescType>
		void InternalAnalyzeSignature(const RootSignatureDescType& Desc, uint32 BindingSpace);

		inline bool HasVisibility(const D3D12_SHADER_VISIBILITY& ParameterVisibility, const D3D12_SHADER_VISIBILITY& Visibility) const
		{
			return ParameterVisibility == D3D12_SHADER_VISIBILITY_ALL || ParameterVisibility == Visibility;
		}

		inline void SetSamplersRDTBindSlot(ShaderType SF, uint8 RootParameterIndex)
		{
			uint8* pBindSlot = nullptr;
			switch (SF)
			{
			case ShaderType::VertexShader: pBindSlot = &BindSlotMap[VS_Samplers]; break;
			case ShaderType::PixelShader: pBindSlot = &BindSlotMap[PS_Samplers]; break;
			case ShaderType::GeometryShader: pBindSlot = &BindSlotMap[GS_Samplers]; break;
			case ShaderType::HullShader: pBindSlot = &BindSlotMap[HS_Samplers]; break;
			case ShaderType::DomainShader: pBindSlot = &BindSlotMap[DS_Samplers]; break;

			case ShaderType::NumStandardType:
			case ShaderType::ComputeShader: pBindSlot = &BindSlotMap[ALL_Samplers]; break;

			default: wconstraint(false);
				return;
			}

			wconstraint(*pBindSlot == 0xFF);
			*pBindSlot = RootParameterIndex;

			bHasSamplers = true;
		}

		inline void SetSRVRDTBindSlot(ShaderType SF, uint8 RootParameterIndex)
		{
			uint8* pBindSlot = nullptr;
			switch (SF)
			{
			case ShaderType::VertexShader: pBindSlot = &BindSlotMap[VS_SRVs]; break;
			case ShaderType::PixelShader: pBindSlot = &BindSlotMap[PS_SRVs]; break;
			case ShaderType::GeometryShader: pBindSlot = &BindSlotMap[GS_SRVs]; break;
			case ShaderType::HullShader: pBindSlot = &BindSlotMap[HS_SRVs]; break;
			case ShaderType::DomainShader: pBindSlot = &BindSlotMap[DS_SRVs]; break;

			case ShaderType::NumStandardType:
			case ShaderType::ComputeShader: pBindSlot = &BindSlotMap[ALL_SRVs]; break;

			default: wconstraint(false);
				return;
			}

			wconstraint(*pBindSlot == 0xFF);
			*pBindSlot = RootParameterIndex;

			bHasSRVs = true;
		}

		inline void SetCBVRDTBindSlot(ShaderType SF, uint8 RootParameterIndex)
		{
			uint8* pBindSlot = nullptr;
			switch (SF)
			{
			case ShaderType::VertexShader: pBindSlot = &BindSlotMap[VS_CBVs]; break;
			case ShaderType::PixelShader: pBindSlot = &BindSlotMap[PS_CBVs]; break;
			case ShaderType::GeometryShader: pBindSlot = &BindSlotMap[GS_CBVs]; break;
			case ShaderType::HullShader: pBindSlot = &BindSlotMap[HS_CBVs]; break;
			case ShaderType::DomainShader: pBindSlot = &BindSlotMap[DS_CBVs]; break;

			case ShaderType::ComputeShader:
			case ShaderType::NumStandardType: pBindSlot = &BindSlotMap[ALL_CBVs]; break;

			default: wconstraint(false);
				return;
			}

			wconstraint(*pBindSlot == 0xFF);
			*pBindSlot = RootParameterIndex;

			bHasCBVs = true;
			bHasRDTCBVs = true;
		}

		inline void SetCBVRDBindSlot(ShaderType SF, uint8 RootParameterIndex)
		{
			uint8* pBindSlot = nullptr;
			switch (SF)
			{
			case ShaderType::VertexShader: pBindSlot = &BindSlotMap[VS_RootCBVs]; break;
			case ShaderType::PixelShader: pBindSlot = &BindSlotMap[PS_RootCBVs]; break;
			case ShaderType::GeometryShader: pBindSlot = &BindSlotMap[GS_RootCBVs]; break;
			case ShaderType::HullShader: pBindSlot = &BindSlotMap[HS_RootCBVs]; break;
			case ShaderType::DomainShader: pBindSlot = &BindSlotMap[DS_RootCBVs]; break;

			case ShaderType::ComputeShader:
			case ShaderType::NumStandardType: pBindSlot = &BindSlotMap[ALL_RootCBVs]; break;

			default: wconstraint(false);
				return;
			}

			wconstraint(*pBindSlot == 0xFF);
			*pBindSlot = RootParameterIndex;

			bHasCBVs = true;
			bHasRDCBVs = true;
		}

		inline void SetUAVRDTBindSlot(ShaderType SF, uint8 RootParameterIndex)
		{
			wconstraint(SF == ShaderType::PixelShader || SF == ShaderType::ComputeShader || SF == ShaderType::NumStandardType);
			uint8* pBindSlot = &BindSlotMap[ALL_UAVs];

			wconstraint(*pBindSlot == 0xFF);
			*pBindSlot = RootParameterIndex;

			bHasUAVs = true;
		}

		inline void SetMaxSamplerCount(ShaderType SF, uint8 Count)
		{
			if (SF == ShaderType::NumStandardType)
			{
				// Update all counts for all stages.
				for (uint32 s = ShaderType::VertexShader; s <= ShaderType::ComputeShader; s++)
				{
					Stage[s].MaxSamplerCount = Count;
				}
			}
			else
			{
				Stage[SF].MaxSamplerCount = Count;
			}
		}

		inline void SetMaxSRVCount(ShaderType SF, uint8 Count)
		{
			if (SF == ShaderType::NumStandardType)
			{
				// Update all counts for all stages.
				for (uint32 s = ShaderType::VertexShader; s <= ShaderType::ComputeShader; s++)
				{
					Stage[s].MaxSRVCount = Count;
				}
			}
			else
			{
				Stage[SF].MaxSRVCount = Count;
			}
		}

		// Update the mask that indicates what shader registers are used in the descriptor table.
		template<typename DescriptorRangeType>
		inline void UpdateCBVRegisterMaskWithDescriptorRange(ShaderType SF, const DescriptorRangeType& Range)
		{
			const uint32 StartRegister = Range.BaseShaderRegister;
			const uint32 EndRegister = StartRegister + Range.NumDescriptors;
			const uint32 StartStage = (SF == ShaderType::NumStandardType) ? ShaderType::VertexShader : SF;
			const uint32 EndStage = (SF == ShaderType::NumStandardType) ? ShaderType::ComputeShader : SF;
			for (uint32 CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage++)
			{
				for (uint32 Register = StartRegister; Register < EndRegister; Register++)
				{
					// The bit shouldn't already be set for the current register.
					wconstraint((Stage[CurrentStage].CBVRegisterMask & (1 << Register)) == 0);
					Stage[CurrentStage].CBVRegisterMask |= (1 << Register);
				}
			}
		}

		// Update the mask that indicates what shader registers are used in the root descriptor.
		template<typename DescriptorType>
		inline void UpdateCBVRegisterMaskWithDescriptor(ShaderType SF, const DescriptorType& Descriptor)
		{
			const uint32 StartStage = (SF == ShaderType::NumStandardType) ? ShaderType::VertexShader : SF;
			const uint32 EndStage = (SF == ShaderType::NumStandardType) ? ShaderType::ComputeShader : SF;
			const uint32& Register = Descriptor.ShaderRegister;
			for (uint32 CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage++)
			{
				// The bit shouldn't already be set for the current register.
				wconstraint((Stage[CurrentStage].CBVRegisterMask & (1 << Register)) == 0);
				Stage[CurrentStage].CBVRegisterMask |= (1 << Register);
			}
		}

		inline void SetMaxCBVCount(ShaderType SF, uint8 Count)
		{
			if (SF == ShaderType::NumStandardType)
			{
				// Update all counts for all stages.
				for (uint32 s = ShaderType::VertexShader; s <= ShaderType::ComputeShader; s++)
				{
					Stage[s].MaxCBVCount = Count;
				}
			}
			else
			{
				Stage[SF].MaxCBVCount = Count;
			}
		}

		inline void IncrementMaxCBVCount(ShaderType SF, uint8 Count)
		{
			if (SF == ShaderType::NumStandardType)
			{
				// Update all counts for all stages.
				for (uint32 s = ShaderType::VertexShader; s <= ShaderType::ComputeShader; s++)
				{
					Stage[s].MaxCBVCount += Count;
				}
			}
			else
			{
				Stage[SF].MaxCBVCount += Count;
			}
		}

		inline void SetMaxUAVCount(ShaderType SF, uint8 Count)
		{
			if (SF == ShaderType::NumStandardType)
			{
				// Update all counts for all stages.
				for (uint32 s = ShaderType::VertexShader; s <= ShaderType::ComputeShader; s++)
				{
					Stage[s].MaxUAVCount = Count;
				}
			}
			else
			{
				Stage[SF].MaxUAVCount = Count;
			}
		}
	private:
		COMPtr<ID3DBlob> SignatureBlob;

	public:
		COMPtr<ID3D12RootSignature> Signature;
		uint8 TotalRootSignatureSizeInDWORDs = 0;

		uint8 BindSlotMap[RPK_RootParameterKeyCount];	// This map uses an enum as a key to lookup the root parameter index
		ShaderStage Stage[ShaderType::NumStandardType];


		bool bHasUAVs;
		bool bHasSRVs;
		bool bHasCBVs;
		//table
		bool bHasRDTCBVs;
		bool bHasRDCBVs;
		bool bHasSamplers;
	};

	

	class RootSignatureMap
	{
	public:
		RootSignatureMap(ID3D12Device* pDevice)
			:Device(pDevice)
		{}

		RootSignature* GetRootSignature(const QuantizedBoundShaderState& QBSS);
	private:
		RootSignature* CreateRootSignature(const QuantizedBoundShaderState& QBSS);

		std::mutex mutex;

		struct KeyHash
		{
			white::uint32 operator()(const QuantizedBoundShaderState& which) const
			{
				return which.GetHashCode();
			}
		};

		ID3D12Device* Device;

		std::unordered_map< QuantizedBoundShaderState,std::unique_ptr<RootSignature>, KeyHash, std::equal_to<QuantizedBoundShaderState>> Map;
	};

	RootSignature* CreateRootSignature(const QuantizedBoundShaderState& QBSS);
}

#endif
