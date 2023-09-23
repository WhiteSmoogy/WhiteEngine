/* D3D12/d3d12_dxgi.h
包装需要动态载入的D3D12函数和声明相关接口以及相关常量定义。
*/
#ifndef WE_RENDER_D3D12_d3d12_dxgi_h
#define WE_RENDER_D3D12_d3d12_dxgi_h 1

#include <WBase/wdef.h>
#include <WBase/winttype.hpp>
#include <WBase/exception_type.h>
#include <WFramework/Win32/WCLib/COM.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dstorage.h>
#include <format>
#include "magic_enum.hpp"

namespace platform_ex {
	namespace Windows {
		namespace D3D {
			template<typename DXCOM>
			void Debug(DXCOM &com, const char * objectname)
			{
#if defined(_DEBUG)
				com->SetPrivateData(::WKPDID_D3DDebugObjectName,static_cast<UINT>(std::strlen(objectname)), objectname);
#endif
			}
			template<typename DXCOM>
			void Debug(DXCOM* com, const char* objectname)
			{
#if defined(_DEBUG)
				com->SetPrivateData(::WKPDID_D3DDebugObjectName, static_cast<UINT>(std::strlen(objectname)), objectname);
#endif
			}
		}

		/*
		\note 桌面平台这些函数是直接通过LoadProc来实现
		\warning 引擎对于这些函数不会也不能频繁调用,无视LoadProc的开销
		\warning 若相应模块未事先载入 抛出Win32Exception
		\todo UWP支持
		*/
		namespace DXGI {
			using namespace white;

			HRESULT CreateFactory2(UINT Flags, REFIID riid, void** ppFactory);

			HRESULT CreateFactory1(REFIID riid, void** ppFactory);
		}
		namespace D3D12 {
			using namespace white;

			HRESULT CreateDevice(IUnknown* pAdapter,
				D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid,
				void** ppDevice);
			HRESULT GetDebugInterface(REFIID riid, void** ppvDebug);
			HRESULT SerializeRootSignature(D3D12_ROOT_SIGNATURE_DESC const * pRootSignature,
				D3D_ROOT_SIGNATURE_VERSION Version, ID3D10Blob** ppBlob, ID3D10Blob** ppErrorBlob);
			HRESULT SerializeVersionedRootSignature(
				 const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
				 ID3DBlob** ppBlob,
				 ID3DBlob** ppErrorBlob);

			struct ResourceStateTransition {
				D3D12_RESOURCE_STATES StateBefore = D3D12_RESOURCE_STATE_COMMON;
				D3D12_RESOURCE_STATES StateAfter = D3D12_RESOURCE_STATE_COMMON;
			};

			struct TransitionBarrier :D3D12_RESOURCE_BARRIER {
				TransitionBarrier(ResourceStateTransition state_trans, COMPtr<ID3D12Resource>& pResource, 
					UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) {
					Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					Flags = flags;
					Transition.pResource = pResource.Get();
					Transition.StateBefore = state_trans.StateBefore;
					Transition.StateAfter = state_trans.StateAfter;

				}

				operator D3D12_RESOURCE_BARRIER*() {
					return this;
				}

				D3D12_RESOURCE_BARRIER* operator!() {
					std::swap(Transition.StateBefore, Transition.StateAfter);
					return this;
				}

			};

			

			inline UINT CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize)
			{
				return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
			}
		}
	}
}

//CD3DX12
namespace platform_ex::Windows::D3D12 {
	constexpr inline struct CD3DX12_DEFAULT {} D3D12_DEFAULT;

	inline constexpr UINT D3D12CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize) noexcept
	{
		return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
	}

	inline UINT8 D3D12GetFormatPlaneCount(
		_In_ ID3D12Device* pDevice,
		DXGI_FORMAT Format
	) noexcept
	{
		D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = { Format, 0 };
		if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(formatInfo))))
		{
			return 0;
		}
		return formatInfo.PlaneCount;
	}

	struct CD3DX12_RECT : public D3D12_RECT
	{
		CD3DX12_RECT()
		{}
		explicit CD3DX12_RECT(const D3D12_RECT& o) :
			D3D12_RECT(o)
		{}
		explicit CD3DX12_RECT(
			LONG Left,
			LONG Top,
			LONG Right,
			LONG Bottom)
		{
			left = Left;
			top = Top;
			right = Right;
			bottom = Bottom;
		}
		~CD3DX12_RECT() {}
		operator const D3D12_RECT& () const { return *this; }
	};

	struct CD3DX12_CLEAR_VALUE : public D3D12_CLEAR_VALUE
	{
		CD3DX12_CLEAR_VALUE()
		{}
		explicit CD3DX12_CLEAR_VALUE(const D3D12_CLEAR_VALUE& o) :
			D3D12_CLEAR_VALUE(o)
		{}
		CD3DX12_CLEAR_VALUE(
			DXGI_FORMAT format,
			const FLOAT color[4])
		{
			Format = format;
			memcpy(Color, color, sizeof(Color));
		}
		CD3DX12_CLEAR_VALUE(
			DXGI_FORMAT format,
			FLOAT depth,
			UINT8 stencil)
		{
			Format = format;
			/* Use memcpy to preserve NAN values */
			memcpy(&DepthStencil.Depth, &depth, sizeof(depth));
			DepthStencil.Stencil = stencil;
		}
		operator const D3D12_CLEAR_VALUE& () const { return *this; }
	};

	struct CD3DX12_DEPTH_STENCIL_DESC1 : public D3D12_DEPTH_STENCIL_DESC1
	{
		CD3DX12_DEPTH_STENCIL_DESC1()
		{}
		explicit CD3DX12_DEPTH_STENCIL_DESC1(const D3D12_DEPTH_STENCIL_DESC1& o) :
			D3D12_DEPTH_STENCIL_DESC1(o)
		{}
		explicit CD3DX12_DEPTH_STENCIL_DESC1(const D3D12_DEPTH_STENCIL_DESC& o)
		{
			DepthEnable = o.DepthEnable;
			DepthWriteMask = o.DepthWriteMask;
			DepthFunc = o.DepthFunc;
			StencilEnable = o.StencilEnable;
			StencilReadMask = o.StencilReadMask;
			StencilWriteMask = o.StencilWriteMask;
			FrontFace.StencilFailOp = o.FrontFace.StencilFailOp;
			FrontFace.StencilDepthFailOp = o.FrontFace.StencilDepthFailOp;
			FrontFace.StencilPassOp = o.FrontFace.StencilPassOp;
			FrontFace.StencilFunc = o.FrontFace.StencilFunc;
			BackFace.StencilFailOp = o.BackFace.StencilFailOp;
			BackFace.StencilDepthFailOp = o.BackFace.StencilDepthFailOp;
			BackFace.StencilPassOp = o.BackFace.StencilPassOp;
			BackFace.StencilFunc = o.BackFace.StencilFunc;
			DepthBoundsTestEnable = FALSE;
		}
		explicit CD3DX12_DEPTH_STENCIL_DESC1(CD3DX12_DEFAULT)
		{
			DepthEnable = TRUE;
			DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			StencilEnable = FALSE;
			StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			FrontFace = defaultStencilOp;
			BackFace = defaultStencilOp;
			DepthBoundsTestEnable = FALSE;
		}
		explicit CD3DX12_DEPTH_STENCIL_DESC1(
			BOOL depthEnable,
			D3D12_DEPTH_WRITE_MASK depthWriteMask,
			D3D12_COMPARISON_FUNC depthFunc,
			BOOL stencilEnable,
			UINT8 stencilReadMask,
			UINT8 stencilWriteMask,
			D3D12_STENCIL_OP frontStencilFailOp,
			D3D12_STENCIL_OP frontStencilDepthFailOp,
			D3D12_STENCIL_OP frontStencilPassOp,
			D3D12_COMPARISON_FUNC frontStencilFunc,
			D3D12_STENCIL_OP backStencilFailOp,
			D3D12_STENCIL_OP backStencilDepthFailOp,
			D3D12_STENCIL_OP backStencilPassOp,
			D3D12_COMPARISON_FUNC backStencilFunc,
			BOOL depthBoundsTestEnable)
		{
			DepthEnable = depthEnable;
			DepthWriteMask = depthWriteMask;
			DepthFunc = depthFunc;
			StencilEnable = stencilEnable;
			StencilReadMask = stencilReadMask;
			StencilWriteMask = stencilWriteMask;
			FrontFace.StencilFailOp = frontStencilFailOp;
			FrontFace.StencilDepthFailOp = frontStencilDepthFailOp;
			FrontFace.StencilPassOp = frontStencilPassOp;
			FrontFace.StencilFunc = frontStencilFunc;
			BackFace.StencilFailOp = backStencilFailOp;
			BackFace.StencilDepthFailOp = backStencilDepthFailOp;
			BackFace.StencilPassOp = backStencilPassOp;
			BackFace.StencilFunc = backStencilFunc;
			DepthBoundsTestEnable = depthBoundsTestEnable;
		}
		~CD3DX12_DEPTH_STENCIL_DESC1() {}
		operator const D3D12_DEPTH_STENCIL_DESC1& () const { return *this; }
		operator D3D12_DEPTH_STENCIL_DESC() const
		{
			D3D12_DEPTH_STENCIL_DESC D;
			D.DepthEnable = DepthEnable;
			D.DepthWriteMask = DepthWriteMask;
			D.DepthFunc = DepthFunc;
			D.StencilEnable = StencilEnable;
			D.StencilReadMask = StencilReadMask;
			D.StencilWriteMask = StencilWriteMask;
			D.FrontFace.StencilFailOp = FrontFace.StencilFailOp;
			D.FrontFace.StencilDepthFailOp = FrontFace.StencilDepthFailOp;
			D.FrontFace.StencilPassOp = FrontFace.StencilPassOp;
			D.FrontFace.StencilFunc = FrontFace.StencilFunc;
			D.BackFace.StencilFailOp = BackFace.StencilFailOp;
			D.BackFace.StencilDepthFailOp = BackFace.StencilDepthFailOp;
			D.BackFace.StencilPassOp = BackFace.StencilPassOp;
			D.BackFace.StencilFunc = BackFace.StencilFunc;
			return D;
		}
	};

	struct CD3DX12_ROOT_CONSTANTS : public D3D12_ROOT_CONSTANTS
	{
		CD3DX12_ROOT_CONSTANTS() {}
		explicit CD3DX12_ROOT_CONSTANTS(const D3D12_ROOT_CONSTANTS& o) :
			D3D12_ROOT_CONSTANTS(o)
		{}
		CD3DX12_ROOT_CONSTANTS(
			UINT num32BitValues,
			UINT shaderRegister,
			UINT registerSpace = 0)
		{
			Init(num32BitValues, shaderRegister, registerSpace);
		}

		inline void Init(
			UINT num32BitValues,
			UINT shaderRegister,
			UINT registerSpace = 0)
		{
			Init(*this, num32BitValues, shaderRegister, registerSpace);
		}

		static inline void Init(
			_Out_ D3D12_ROOT_CONSTANTS& rootConstants,
			UINT num32BitValues,
			UINT shaderRegister,
			UINT registerSpace = 0)
		{
			rootConstants.Num32BitValues = num32BitValues;
			rootConstants.ShaderRegister = shaderRegister;
			rootConstants.RegisterSpace = registerSpace;
		}
	};

	struct CD3DX12_ROOT_SIGNATURE_DESC : public D3D12_ROOT_SIGNATURE_DESC
	{
		CD3DX12_ROOT_SIGNATURE_DESC() {}
		explicit CD3DX12_ROOT_SIGNATURE_DESC(const D3D12_ROOT_SIGNATURE_DESC& o) :
			D3D12_ROOT_SIGNATURE_DESC(o)
		{}
		CD3DX12_ROOT_SIGNATURE_DESC(
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			Init(numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
		}
		CD3DX12_ROOT_SIGNATURE_DESC(CD3DX12_DEFAULT)
		{
			Init(0, NULL, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		}

		inline void Init(
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			Init(*this, numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
		}

		static inline void Init(
			_Out_ D3D12_ROOT_SIGNATURE_DESC& desc,
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			desc.NumParameters = numParameters;
			desc.pParameters = _pParameters;
			desc.NumStaticSamplers = numStaticSamplers;
			desc.pStaticSamplers = _pStaticSamplers;
			desc.Flags = flags;
		}
	};

	struct CD3DX12_DESCRIPTOR_RANGE1 : public D3D12_DESCRIPTOR_RANGE1
	{
		CD3DX12_DESCRIPTOR_RANGE1() { }
		explicit CD3DX12_DESCRIPTOR_RANGE1(const D3D12_DESCRIPTOR_RANGE1& o) :
			D3D12_DESCRIPTOR_RANGE1(o)
		{}
		CD3DX12_DESCRIPTOR_RANGE1(
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			UINT numDescriptors,
			UINT baseShaderRegister,
			UINT registerSpace = 0,
			D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
			UINT offsetInDescriptorsFromTableStart =
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
		{
			Init(rangeType, numDescriptors, baseShaderRegister, registerSpace, flags, offsetInDescriptorsFromTableStart);
		}

		inline void Init(
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			UINT numDescriptors,
			UINT baseShaderRegister,
			UINT registerSpace = 0,
			D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
			UINT offsetInDescriptorsFromTableStart =
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
		{
			Init(*this, rangeType, numDescriptors, baseShaderRegister, registerSpace, flags, offsetInDescriptorsFromTableStart);
		}

		static inline void Init(
			_Out_ D3D12_DESCRIPTOR_RANGE1& range,
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			UINT numDescriptors,
			UINT baseShaderRegister,
			UINT registerSpace = 0,
			D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
			UINT offsetInDescriptorsFromTableStart =
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
		{
			range.RangeType = rangeType;
			range.NumDescriptors = numDescriptors;
			range.BaseShaderRegister = baseShaderRegister;
			range.RegisterSpace = registerSpace;
			range.Flags = flags;
			range.OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
		}
	};

	struct CD3DX12_ROOT_DESCRIPTOR_TABLE1 : public D3D12_ROOT_DESCRIPTOR_TABLE1
	{
		CD3DX12_ROOT_DESCRIPTOR_TABLE1() {}
		explicit CD3DX12_ROOT_DESCRIPTOR_TABLE1(const D3D12_ROOT_DESCRIPTOR_TABLE1& o) :
			D3D12_ROOT_DESCRIPTOR_TABLE1(o)
		{}
		CD3DX12_ROOT_DESCRIPTOR_TABLE1(
			UINT numDescriptorRanges,
			_In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE1* _pDescriptorRanges)
		{
			Init(numDescriptorRanges, _pDescriptorRanges);
		}

		inline void Init(
			UINT numDescriptorRanges,
			_In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE1* _pDescriptorRanges)
		{
			Init(*this, numDescriptorRanges, _pDescriptorRanges);
		}

		static inline void Init(
			_Out_ D3D12_ROOT_DESCRIPTOR_TABLE1& rootDescriptorTable,
			UINT numDescriptorRanges,
			_In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE1* _pDescriptorRanges)
		{
			rootDescriptorTable.NumDescriptorRanges = numDescriptorRanges;
			rootDescriptorTable.pDescriptorRanges = _pDescriptorRanges;
		}
	};

	struct CD3DX12_ROOT_DESCRIPTOR1 : public D3D12_ROOT_DESCRIPTOR1
	{
		CD3DX12_ROOT_DESCRIPTOR1() {}
		explicit CD3DX12_ROOT_DESCRIPTOR1(const D3D12_ROOT_DESCRIPTOR1& o) :
			D3D12_ROOT_DESCRIPTOR1(o)
		{}
		CD3DX12_ROOT_DESCRIPTOR1(
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
		{
			Init(shaderRegister, registerSpace, flags);
		}

		inline void Init(
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
		{
			Init(*this, shaderRegister, registerSpace, flags);
		}

		static inline void Init(
			_Out_ D3D12_ROOT_DESCRIPTOR1& table,
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
		{
			table.ShaderRegister = shaderRegister;
			table.RegisterSpace = registerSpace;
			table.Flags = flags;
		}
	};

	struct CD3DX12_ROOT_PARAMETER1 : public D3D12_ROOT_PARAMETER1
	{
		CD3DX12_ROOT_PARAMETER1() {}
		explicit CD3DX12_ROOT_PARAMETER1(const D3D12_ROOT_PARAMETER1& o) :
			D3D12_ROOT_PARAMETER1(o)
		{}

		static inline void InitAsDescriptorTable(
			_Out_ D3D12_ROOT_PARAMETER1& rootParam,
			UINT numDescriptorRanges,
			_In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParam.ShaderVisibility = visibility;
			CD3DX12_ROOT_DESCRIPTOR_TABLE1::Init(rootParam.DescriptorTable, numDescriptorRanges, pDescriptorRanges);
		}

		static inline void InitAsConstants(
			_Out_ D3D12_ROOT_PARAMETER1& rootParam,
			UINT num32BitValues,
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParam.ShaderVisibility = visibility;
			CD3DX12_ROOT_CONSTANTS::Init(rootParam.Constants, num32BitValues, shaderRegister, registerSpace);
		}

		static inline void InitAsConstantBufferView(
			_Out_ D3D12_ROOT_PARAMETER1& rootParam,
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParam.ShaderVisibility = visibility;
			CD3DX12_ROOT_DESCRIPTOR1::Init(rootParam.Descriptor, shaderRegister, registerSpace, flags);
		}

		static inline void InitAsShaderResourceView(
			_Out_ D3D12_ROOT_PARAMETER1& rootParam,
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			rootParam.ShaderVisibility = visibility;
			CD3DX12_ROOT_DESCRIPTOR1::Init(rootParam.Descriptor, shaderRegister, registerSpace, flags);
		}

		static inline void InitAsUnorderedAccessView(
			_Out_ D3D12_ROOT_PARAMETER1& rootParam,
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			rootParam.ShaderVisibility = visibility;
			CD3DX12_ROOT_DESCRIPTOR1::Init(rootParam.Descriptor, shaderRegister, registerSpace, flags);
		}

		inline void InitAsDescriptorTable(
			UINT numDescriptorRanges,
			_In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsDescriptorTable(*this, numDescriptorRanges, pDescriptorRanges, visibility);
		}

		inline void InitAsConstants(
			UINT num32BitValues,
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsConstants(*this, num32BitValues, shaderRegister, registerSpace, visibility);
		}

		inline void InitAsConstantBufferView(
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsConstantBufferView(*this, shaderRegister, registerSpace, flags, visibility);
		}

		inline void InitAsShaderResourceView(
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsShaderResourceView(*this, shaderRegister, registerSpace, flags, visibility);
		}

		inline void InitAsUnorderedAccessView(
			UINT shaderRegister,
			UINT registerSpace = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsUnorderedAccessView(*this, shaderRegister, registerSpace, flags, visibility);
		}
	};

	struct CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC : public D3D12_VERSIONED_ROOT_SIGNATURE_DESC
	{
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC() {}
		explicit CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& o) :
			D3D12_VERSIONED_ROOT_SIGNATURE_DESC(o)
		{}
		explicit CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(const D3D12_ROOT_SIGNATURE_DESC& o)
		{
			Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
			Desc_1_0 = o;
		}
		explicit CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(const D3D12_ROOT_SIGNATURE_DESC1& o)
		{
			Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			Desc_1_1 = o;
		}
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			Init_1_0(numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
		}
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER1* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			Init_1_1(numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
		}
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(CD3DX12_DEFAULT)
		{
			Init_1_1(0, NULL, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		}

		inline void Init_1_0(
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			Init_1_0(*this, numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
		}

		static inline void Init_1_0(
			_Out_ D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc,
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
			desc.Desc_1_0.NumParameters = numParameters;
			desc.Desc_1_0.pParameters = _pParameters;
			desc.Desc_1_0.NumStaticSamplers = numStaticSamplers;
			desc.Desc_1_0.pStaticSamplers = _pStaticSamplers;
			desc.Desc_1_0.Flags = flags;
		}

		inline void Init_1_1(
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER1* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			Init_1_1(*this, numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
		}

		static inline void Init_1_1(
			_Out_ D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc,
			UINT numParameters,
			_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER1* _pParameters,
			UINT numStaticSamplers = 0,
			_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
			D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
		{
			desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			desc.Desc_1_1.NumParameters = numParameters;
			desc.Desc_1_1.pParameters = _pParameters;
			desc.Desc_1_1.NumStaticSamplers = numStaticSamplers;
			desc.Desc_1_1.pStaticSamplers = _pStaticSamplers;
			desc.Desc_1_1.Flags = flags;
		}
	};

	struct CD3DX12_CPU_DESCRIPTOR_HANDLE : public D3D12_CPU_DESCRIPTOR_HANDLE
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE() {}
		explicit CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& o) :
			D3D12_CPU_DESCRIPTOR_HANDLE(o)
		{}
		CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT) { ptr = 0; }
		CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetScaledByIncrementSize)
		{
			InitOffsetted(other, offsetScaledByIncrementSize);
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			InitOffsetted(other, offsetInDescriptors, descriptorIncrementSize);
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			ptr += offsetInDescriptors * descriptorIncrementSize;
			return *this;
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize)
		{
			ptr += offsetScaledByIncrementSize;
			return *this;
		}
		bool operator==(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other) const
		{
			return (ptr == other.ptr);
		}
		bool operator!=(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other) const
		{
			return (ptr != other.ptr);
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE& operator=(const D3D12_CPU_DESCRIPTOR_HANDLE& other)
		{
			ptr = other.ptr;
			return *this;
		}

		inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
		{
			InitOffsetted(*this, base, offsetScaledByIncrementSize);
		}

		inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			InitOffsetted(*this, base, offsetInDescriptors, descriptorIncrementSize);
		}

		static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
		{
			handle.ptr = base.ptr + offsetScaledByIncrementSize;
		}

		static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			handle.ptr = base.ptr + offsetInDescriptors * descriptorIncrementSize;
		}
	};

	//------------------------------------------------------------------------------------------------
	struct CD3DX12_GPU_DESCRIPTOR_HANDLE : public D3D12_GPU_DESCRIPTOR_HANDLE
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE() {}
		explicit CD3DX12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE& o) :
			D3D12_GPU_DESCRIPTOR_HANDLE(o)
		{}
		CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT) { ptr = 0; }
		CD3DX12_GPU_DESCRIPTOR_HANDLE(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& other, INT offsetScaledByIncrementSize)
		{
			InitOffsetted(other, offsetScaledByIncrementSize);
		}
		CD3DX12_GPU_DESCRIPTOR_HANDLE(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& other, INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			InitOffsetted(other, offsetInDescriptors, descriptorIncrementSize);
		}
		CD3DX12_GPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			ptr += offsetInDescriptors * descriptorIncrementSize;
			return *this;
		}
		CD3DX12_GPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize)
		{
			ptr += offsetScaledByIncrementSize;
			return *this;
		}
		inline bool operator==(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& other) const
		{
			return (ptr == other.ptr);
		}
		inline bool operator!=(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& other) const
		{
			return (ptr != other.ptr);
		}
		CD3DX12_GPU_DESCRIPTOR_HANDLE& operator=(const D3D12_GPU_DESCRIPTOR_HANDLE& other)
		{
			ptr = other.ptr;
			return *this;
		}

		inline void InitOffsetted(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
		{
			InitOffsetted(*this, base, offsetScaledByIncrementSize);
		}

		inline void InitOffsetted(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			InitOffsetted(*this, base, offsetInDescriptors, descriptorIncrementSize);
		}

		static inline void InitOffsetted(_Out_ D3D12_GPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
		{
			handle.ptr = base.ptr + offsetScaledByIncrementSize;
		}

		static inline void InitOffsetted(_Out_ D3D12_GPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
		{
			handle.ptr = base.ptr + offsetInDescriptors * descriptorIncrementSize;
		}
	};

	//------------------------------------------------------------------------------------------------
	struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC
	{
		CD3DX12_RESOURCE_DESC() = default;
		explicit CD3DX12_RESOURCE_DESC(const D3D12_RESOURCE_DESC& o) noexcept :
			D3D12_RESOURCE_DESC(o)
		{}
		CD3DX12_RESOURCE_DESC(
			D3D12_RESOURCE_DIMENSION dimension,
			UINT64 alignment,
			UINT64 width,
			UINT height,
			UINT16 depthOrArraySize,
			UINT16 mipLevels,
			DXGI_FORMAT format,
			UINT sampleCount,
			UINT sampleQuality,
			D3D12_TEXTURE_LAYOUT layout,
			D3D12_RESOURCE_FLAGS flags) noexcept
		{
			Dimension = dimension;
			Alignment = alignment;
			Width = width;
			Height = height;
			DepthOrArraySize = depthOrArraySize;
			MipLevels = mipLevels;
			Format = format;
			SampleDesc.Count = sampleCount;
			SampleDesc.Quality = sampleQuality;
			Layout = layout;
			Flags = flags;
		}
		static inline CD3DX12_RESOURCE_DESC Buffer(
			const D3D12_RESOURCE_ALLOCATION_INFO& resAllocInfo,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) noexcept
		{
			return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, resAllocInfo.Alignment, resAllocInfo.SizeInBytes,
				1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
		}
		static inline CD3DX12_RESOURCE_DESC Buffer(
			UINT64 width,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
			UINT64 alignment = 0) noexcept
		{
			return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1,
				DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
		}
		static inline CD3DX12_RESOURCE_DESC Tex1D(
			DXGI_FORMAT format,
			UINT64 width,
			UINT16 arraySize = 1,
			UINT16 mipLevels = 0,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
			D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			UINT64 alignment = 0) noexcept
		{
			return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE1D, alignment, width, 1, arraySize,
				mipLevels, format, 1, 0, layout, flags);
		}
		static inline CD3DX12_RESOURCE_DESC Tex2D(
			DXGI_FORMAT format,
			UINT64 width,
			UINT height,
			UINT16 arraySize = 1,
			UINT16 mipLevels = 0,
			UINT sampleCount = 1,
			UINT sampleQuality = 0,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
			D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			UINT64 alignment = 0) noexcept
		{
			return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE2D, alignment, width, height, arraySize,
				mipLevels, format, sampleCount, sampleQuality, layout, flags);
		}
		static inline CD3DX12_RESOURCE_DESC Tex3D(
			DXGI_FORMAT format,
			UINT64 width,
			UINT height,
			UINT16 depth,
			UINT16 mipLevels = 0,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
			D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			UINT64 alignment = 0) noexcept
		{
			return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE3D, alignment, width, height, depth,
				mipLevels, format, 1, 0, layout, flags);
		}
		inline UINT16 Depth() const noexcept
		{
			return (Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1);
		}
		inline UINT16 ArraySize() const noexcept
		{
			return (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1);
		}
		inline UINT8 PlaneCount(_In_ ID3D12Device* pDevice) const noexcept
		{
			return D3D12GetFormatPlaneCount(pDevice, Format);
		}
		inline UINT Subresources(_In_ ID3D12Device* pDevice) const noexcept
		{
			return MipLevels * ArraySize() * PlaneCount(pDevice);
		}
		inline UINT CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice) noexcept
		{
			return D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, MipLevels, ArraySize());
		}
	};

	//------------------------------------------------------------------------------------------------
	struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES
	{
		CD3DX12_HEAP_PROPERTIES() = default;
		explicit CD3DX12_HEAP_PROPERTIES(const D3D12_HEAP_PROPERTIES& o) noexcept :
			D3D12_HEAP_PROPERTIES(o)
		{}
		CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY cpuPageProperty,
			D3D12_MEMORY_POOL memoryPoolPreference,
			UINT creationNodeMask = 1,
			UINT nodeMask = 1) noexcept
		{
			Type = D3D12_HEAP_TYPE_CUSTOM;
			CPUPageProperty = cpuPageProperty;
			MemoryPoolPreference = memoryPoolPreference;
			CreationNodeMask = creationNodeMask;
			VisibleNodeMask = nodeMask;
		}
		explicit CD3DX12_HEAP_PROPERTIES(
			D3D12_HEAP_TYPE type,
			UINT creationNodeMask = 1,
			UINT nodeMask = 1) noexcept
		{
			Type = type;
			CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			CreationNodeMask = creationNodeMask;
			VisibleNodeMask = nodeMask;
		}
		bool IsCPUAccessible() const noexcept
		{
			return Type == D3D12_HEAP_TYPE_UPLOAD || Type == D3D12_HEAP_TYPE_READBACK || (Type == D3D12_HEAP_TYPE_CUSTOM &&
				(CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE || CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK));
		}
	};

	// Row-by-row memcpy
	inline void MemcpySubresource(
		_In_ const D3D12_MEMCPY_DEST* pDest,
		_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
		SIZE_T RowSizeInBytes,
		UINT NumRows,
		UINT NumSlices) noexcept
	{
		for (UINT z = 0; z < NumSlices; ++z)
		{
			auto pDestSlice = static_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
			auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
			for (UINT y = 0; y < NumRows; ++y)
			{
				memcpy(pDestSlice + pDest->RowPitch * y,
					pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
					RowSizeInBytes);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// D3D12 exports a new method for serializing root signatures in the Windows 10 Anniversary Update.
	// To help enable root signature 1.1 features when they are available and not require maintaining
	// two code paths for building root signatures, this helper method reconstructs a 1.0 signature when
	// 1.1 is not supported.
	inline HRESULT D3DX12SerializeVersionedRootSignature(
		_In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION MaxVersion,
		_Outptr_ ID3DBlob** ppBlob,
		_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob)
	{
		if (ppErrorBlob != NULL)
		{
			*ppErrorBlob = NULL;
		}

		switch (MaxVersion)
		{
		case D3D_ROOT_SIGNATURE_VERSION_1_0:
			switch (pRootSignatureDesc->Version)
			{
			case D3D_ROOT_SIGNATURE_VERSION_1_0:
				return SerializeRootSignature(&pRootSignatureDesc->Desc_1_0, D3D_ROOT_SIGNATURE_VERSION_1, ppBlob, ppErrorBlob);

			case D3D_ROOT_SIGNATURE_VERSION_1_1:
			{
				HRESULT hr = S_OK;
				const D3D12_ROOT_SIGNATURE_DESC1& desc_1_1 = pRootSignatureDesc->Desc_1_1;

				const SIZE_T ParametersSize = sizeof(D3D12_ROOT_PARAMETER) * desc_1_1.NumParameters;
				void* pParameters = (ParametersSize > 0) ? HeapAlloc(GetProcessHeap(), 0, ParametersSize) : NULL;
				if (ParametersSize > 0 && pParameters == NULL)
				{
					hr = E_OUTOFMEMORY;
				}
				D3D12_ROOT_PARAMETER* pParameters_1_0 = reinterpret_cast<D3D12_ROOT_PARAMETER*>(pParameters);

				if (SUCCEEDED(hr))
				{
					for (UINT n = 0; n < desc_1_1.NumParameters; n++)
					{
						__analysis_assume(ParametersSize == sizeof(D3D12_ROOT_PARAMETER) * desc_1_1.NumParameters);
						pParameters_1_0[n].ParameterType = desc_1_1.pParameters[n].ParameterType;
						pParameters_1_0[n].ShaderVisibility = desc_1_1.pParameters[n].ShaderVisibility;

						switch (desc_1_1.pParameters[n].ParameterType)
						{
						case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
							pParameters_1_0[n].Constants.Num32BitValues = desc_1_1.pParameters[n].Constants.Num32BitValues;
							pParameters_1_0[n].Constants.RegisterSpace = desc_1_1.pParameters[n].Constants.RegisterSpace;
							pParameters_1_0[n].Constants.ShaderRegister = desc_1_1.pParameters[n].Constants.ShaderRegister;
							break;

						case D3D12_ROOT_PARAMETER_TYPE_CBV:
						case D3D12_ROOT_PARAMETER_TYPE_SRV:
						case D3D12_ROOT_PARAMETER_TYPE_UAV:
							pParameters_1_0[n].Descriptor.RegisterSpace = desc_1_1.pParameters[n].Descriptor.RegisterSpace;
							pParameters_1_0[n].Descriptor.ShaderRegister = desc_1_1.pParameters[n].Descriptor.ShaderRegister;
							break;

						case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
							const D3D12_ROOT_DESCRIPTOR_TABLE1& table_1_1 = desc_1_1.pParameters[n].DescriptorTable;

							const SIZE_T DescriptorRangesSize = sizeof(D3D12_DESCRIPTOR_RANGE) * table_1_1.NumDescriptorRanges;
							void* pDescriptorRanges = (DescriptorRangesSize > 0 && SUCCEEDED(hr)) ? HeapAlloc(GetProcessHeap(), 0, DescriptorRangesSize) : NULL;
							if (DescriptorRangesSize > 0 && pDescriptorRanges == NULL)
							{
								hr = E_OUTOFMEMORY;
							}
							D3D12_DESCRIPTOR_RANGE* pDescriptorRanges_1_0 = reinterpret_cast<D3D12_DESCRIPTOR_RANGE*>(pDescriptorRanges);

							if (SUCCEEDED(hr))
							{
								for (UINT x = 0; x < table_1_1.NumDescriptorRanges; x++)
								{
									__analysis_assume(DescriptorRangesSize == sizeof(D3D12_DESCRIPTOR_RANGE) * table_1_1.NumDescriptorRanges);
									pDescriptorRanges_1_0[x].BaseShaderRegister = table_1_1.pDescriptorRanges[x].BaseShaderRegister;
									pDescriptorRanges_1_0[x].NumDescriptors = table_1_1.pDescriptorRanges[x].NumDescriptors;
									pDescriptorRanges_1_0[x].OffsetInDescriptorsFromTableStart = table_1_1.pDescriptorRanges[x].OffsetInDescriptorsFromTableStart;
									pDescriptorRanges_1_0[x].RangeType = table_1_1.pDescriptorRanges[x].RangeType;
									pDescriptorRanges_1_0[x].RegisterSpace = table_1_1.pDescriptorRanges[x].RegisterSpace;
								}
							}

							D3D12_ROOT_DESCRIPTOR_TABLE& table_1_0 = pParameters_1_0[n].DescriptorTable;
							table_1_0.NumDescriptorRanges = table_1_1.NumDescriptorRanges;
							table_1_0.pDescriptorRanges = pDescriptorRanges_1_0;
						}
					}
				}

				if (SUCCEEDED(hr))
				{
					CD3DX12_ROOT_SIGNATURE_DESC desc_1_0(desc_1_1.NumParameters, pParameters_1_0, desc_1_1.NumStaticSamplers, desc_1_1.pStaticSamplers, desc_1_1.Flags);
					hr = SerializeRootSignature(&desc_1_0, D3D_ROOT_SIGNATURE_VERSION_1, ppBlob, ppErrorBlob);
				}

				if (pParameters)
				{
					for (UINT n = 0; n < desc_1_1.NumParameters; n++)
					{
						if (desc_1_1.pParameters[n].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
						{
							HeapFree(GetProcessHeap(), 0, reinterpret_cast<void*>(const_cast<D3D12_DESCRIPTOR_RANGE*>(pParameters_1_0[n].DescriptorTable.pDescriptorRanges)));
						}
					}
					HeapFree(GetProcessHeap(), 0, pParameters);
				}
				return hr;
			}
			}
			break;

		case D3D_ROOT_SIGNATURE_VERSION_1_1:
			return SerializeVersionedRootSignature(pRootSignatureDesc, ppBlob, ppErrorBlob);
		}

		return E_INVALIDARG;
	}
}

namespace platform_ex::Windows::D3D12
{
	inline bool IsCPUWritable(D3D12_HEAP_TYPE HeapType, const D3D12_HEAP_PROPERTIES* pCustomHeapProperties = nullptr)
	{
		wconstraint(HeapType == D3D12_HEAP_TYPE_CUSTOM ? pCustomHeapProperties != nullptr : true);
		return HeapType == D3D12_HEAP_TYPE_UPLOAD ||
			(HeapType == D3D12_HEAP_TYPE_CUSTOM &&
				(pCustomHeapProperties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE || pCustomHeapProperties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK));
	}

	inline bool IsGPUOnly(D3D12_HEAP_TYPE HeapType, const D3D12_HEAP_PROPERTIES* pCustomHeapProperties = nullptr)
	{
		wconstraint(HeapType == D3D12_HEAP_TYPE_CUSTOM ? pCustomHeapProperties != nullptr : true);
		return HeapType == D3D12_HEAP_TYPE_DEFAULT ||
			(HeapType == D3D12_HEAP_TYPE_CUSTOM &&
				(pCustomHeapProperties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE));
	}

	inline bool IsCPUAccessible(D3D12_HEAP_TYPE HeapType, const D3D12_HEAP_PROPERTIES* pCustomHeapProperties = nullptr)
	{
		return !IsGPUOnly(HeapType, pCustomHeapProperties);
	}

	inline D3D12_RESOURCE_STATES DetermineInitialResourceState(D3D12_HEAP_TYPE HeapType, const D3D12_HEAP_PROPERTIES* pCustomHeapProperties = nullptr)
	{
		if (HeapType == D3D12_HEAP_TYPE_DEFAULT || IsCPUWritable(HeapType, pCustomHeapProperties))
		{
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else
		{
			wconstraint(HeapType == D3D12_HEAP_TYPE_READBACK);
			return D3D12_RESOURCE_STATE_COPY_DEST;
		}
	}

	inline DXGI_FORMAT FindUnorderedAccessDXGIFormat(DXGI_FORMAT InFormat)
	{
		switch (InFormat)
		{
		case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		return InFormat;
	}

	inline uint32 GetFormatSizeInBits(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		default: throw white::unsupported(); [[fallthrough]];
		case DXGI_FORMAT_UNKNOWN:
			return 0;

		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_Y416:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:
		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
		case DXGI_FORMAT_YUY2:
			return 32;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_NV11:
			return 16;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
			return 8;

		case DXGI_FORMAT_R1_UNORM:
			return 1;
		}
	}

	inline uint32 GetFormatSizeInBytes(DXGI_FORMAT Format)
	{
		return GetFormatSizeInBits(Format) / 8;
	}
}

//const def
namespace platform_ex::Windows::D3D12 {
	// DX12 doesn't support higher MSAA count
	constexpr unsigned DX_MAX_MSAA_COUNT = 8;

	// This is a value that should be tweaked to fit the app, lower numbers will have better performance
	// Titles using many terrain layers may want to set MAX_SRVS to 64 to avoid shader compilation errors. This will have a small performance hit of around 0.1%
	constexpr unsigned MAX_SRVS = 48;
	constexpr unsigned MAX_SAMPLERS = 16;
	constexpr unsigned MAX_UAVS = 16;
	constexpr unsigned MAX_CBS = 16;

	// This value controls how many root constant buffers can be used per shader stage in a root signature.
	// Note: Using root descriptors significantly increases the size of root signatures (each root descriptor is 2 DWORDs).
	constexpr unsigned MAX_ROOT_CBVS = MAX_CBS;

	constexpr DXGI_COLOR_SPACE_TYPE DXGI_HDR_ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

	using CBVSlotMask = white::make_width_int<MAX_CBS>::unsigned_least_type;

	constexpr CBVSlotMask GRootCBVSlotMask = (1 << MAX_ROOT_CBVS) - 1; // Mask for all slots that are used by root descriptors.
	constexpr CBVSlotMask GDescriptorTableCBVSlotMask = static_cast<CBVSlotMask>(-1) & ~(GRootCBVSlotMask); // Mask for all slots that are used by a root descriptor table.

	using SRVSlotMask = white::make_width_int<MAX_SRVS>::unsigned_fast_type;

	using SamplerSlotMask = white::make_width_int<MAX_SAMPLERS>::unsigned_type;

	using UAVSlotMask = white::make_width_int<MAX_UAVS>::unsigned_type;

	constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_TBD = D3D12_RESOURCE_STATES(-1 ^ (1 << 31));
	constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_CORRUPT = D3D12_RESOURCE_STATES(-2 ^ (1 << 31));
}

template<typename CharT>
struct std::formatter<D3D12_HEAP_TYPE, CharT> : std::formatter<std::string_view, CharT>
{
	template <class FormatContext>
	typename FormatContext::iterator format(D3D12_HEAP_TYPE heapType, FormatContext& ctx) const
	{
		return std::formatter<std::string_view, CharT>::format(magic_enum::enum_name(heapType), ctx);
	}
};

#if WFL_Win64
#ifndef USE_PIX
#define USE_PIX 1
#endif
#endif

#ifndef ENABLE_AFTER_MATH
#define ENABLE_AFTER_MATH 1
#endif

#if ENABLE_AFTER_MATH
extern int GEnableNvidaiAfterMath;
#endif

#endif