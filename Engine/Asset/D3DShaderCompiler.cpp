#include <WFramework/WCLib/Platform.h>
#include <WFramework/Win32/WCLib/COM.h>
#include "WFramework/Helper/ShellHelper.h"
#include <WFramework/Core/WString.h>
#include <WFramework/Win32/WCLib/Mingw32.h>
#include <WFramework/Win32/WCLib/NLS.h>
#include <WBase/ConcurrentHashMap.h>
#include "D3DShaderCompiler.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/RayTracingDefinitions.h"
#include "RenderInterface/BuiltInRayTracingShader.h"
#include "Runtime/Core/LFile.h"
#include "Runtime/Core/Path.h"
#include "spdlog/spdlog.h"

#include "../emacro.h"

#include <filesystem>

#include <algorithm>

using namespace platform::Render::Shader;
using namespace platform_ex;
using namespace D3DFlags;

#define SHADER_OPTIMIZATION_LEVEL_MASK (D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_OPTIMIZATION_LEVEL2 | D3DCOMPILE_OPTIMIZATION_LEVEL3)

namespace asset::X::Shader {
	void AppendCompilerEnvironment(FShaderCompilerEnvironment& environment, ShaderType type)
	{
		using namespace  platform::Render;

		switch (type) {
		case ShaderType::VertexShader:
			environment.SetDefine("VS", "1");
			break;
		case ShaderType::PixelShader:
			environment.SetDefine("PS", "1");
			break;
		}
	}

	std::string_view CompileDXBCProfile(ShaderType type)
	{
		using namespace  platform::Render;

		switch (type) {
		case ShaderType::VertexShader:
			return "vs_5_0";
		case ShaderType::PixelShader:
			return "ps_5_0";
		case ShaderType::GeometryShader:
			return "gs_5_0";
		case ShaderType::ComputeShader:
			return "cs_5_0";
		}

		return "";
	}

	std::string_view CompileDXILProfile(ShaderType type)
	{
		using namespace  platform::Render;

		switch (type) {
		case ShaderType::VertexShader:
			return "vs_6_6";
		case ShaderType::PixelShader:
			return "ps_6_6";
		case ShaderType::GeometryShader:
			return "gs_6_6";
		case ShaderType::ComputeShader:
			return "cs_6_6";
		case ShaderType::RayGen:
		case ShaderType::RayMiss:
		case ShaderType::RayHitGroup:
		case ShaderType::RayCallable:
			return "lib_6_6";
		}

		return "";
	}
}



#ifdef WFL_Win32
#include <d3dcompiler.h>
#ifdef WFL_Win64
#pragma comment(lib,"d3dcompiler.lib")
#else
#pragma comment(lib,"d3dcompiler.lib")
#endif
#include <dxc/Support/dxcapi.use.h>

template<typename T>
void UniqueAddByName(std::vector<T>& target, T&& value)
{
	for (auto& compare : target)
	{
		if (compare.name == value.name)
			return;
	}

	target.emplace_back(std::move(value));
}

template<typename ID3D1xShaderReflection, typename D3D1x_SHADER_DESC>
void FillD3D12Reflect(ID3D1xShaderReflection* pReflection, ShaderInfo* pInfo, ShaderType type)
{
	D3D1x_SHADER_DESC desc;
	pReflection->GetDesc(&desc);

	for (UINT i = 0; i != desc.ConstantBuffers; ++i) {
		auto pReflectionConstantBuffer = pReflection->GetConstantBufferByIndex(i);

		D3D12_SHADER_BUFFER_DESC buffer_desc;
		pReflectionConstantBuffer->GetDesc(&buffer_desc);
		if ((D3D_CT_CBUFFER == buffer_desc.Type) || (D3D_CT_TBUFFER == buffer_desc.Type)) {
			ShaderInfo::ConstantBufferInfo  ConstantBufferInfo;
			ConstantBufferInfo.name = buffer_desc.Name;
			ConstantBufferInfo.name_hash = white::constfn_hash(buffer_desc.Name);
			ConstantBufferInfo.size = buffer_desc.Size;
			ConstantBufferInfo.bind_point = i;

			for (UINT v = 0; v != buffer_desc.Variables; ++v) {
				auto pReflectionVar = pReflectionConstantBuffer->GetVariableByIndex(v);
				D3D12_SHADER_VARIABLE_DESC variable_desc;
				pReflectionVar->GetDesc(&variable_desc);

				D3D12_SHADER_TYPE_DESC type_desc;
				pReflectionVar->GetType()->GetDesc(&type_desc);

				ShaderInfo::ConstantBufferInfo::VariableInfo VariableInfo;
				VariableInfo.name = variable_desc.Name;
				VariableInfo.start_offset = variable_desc.StartOffset;
				VariableInfo.type = variable_desc.StartOffset;
				VariableInfo.rows = variable_desc.StartOffset;
				VariableInfo.columns = variable_desc.StartOffset;
				VariableInfo.elements = variable_desc.StartOffset;
				VariableInfo.size = variable_desc.Size;

				ConstantBufferInfo.var_desc.emplace_back(std::move(VariableInfo));
			}
			UniqueAddByName(pInfo->ConstantBufferInfos, std::move(ConstantBufferInfo));
		}
	}
	pInfo->ResourceCounts.NumCBs = static_cast<white::uint16>(pInfo->ConstantBufferInfos.size());

	for (UINT i = 0; i != desc.BoundResources; ++i) {
		D3D12_SHADER_INPUT_BIND_DESC input_bind_desc;
		pReflection->GetResourceBindingDesc(i, &input_bind_desc);

		auto BindPoint = static_cast<white::uint16>(input_bind_desc.BindPoint + 1);
		switch (input_bind_desc.Type)
		{
		case D3D_SIT_SAMPLER:
			pInfo->ResourceCounts.NumSamplers = std::max(pInfo->ResourceCounts.NumSamplers, BindPoint);
			break;

		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			pInfo->ResourceCounts.NumSRVs = std::max(pInfo->ResourceCounts.NumSRVs, BindPoint);
			break;

		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			pInfo->ResourceCounts.NumUAVs = std::max(pInfo->ResourceCounts.NumUAVs, BindPoint);
			break;

		default:
			break;
		}

		switch (input_bind_desc.Type)
		{
		case D3D_SIT_TEXTURE:
		case D3D_SIT_SAMPLER:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		{
			ShaderInfo::BoundResourceInfo BoundResourceInfo;
			BoundResourceInfo.name = input_bind_desc.Name;
			BoundResourceInfo.type = static_cast<uint8_t>(input_bind_desc.Type);
			BoundResourceInfo.bind_point = static_cast<uint16_t>(input_bind_desc.BindPoint);
			UniqueAddByName(pInfo->BoundResourceInfos, std::move(BoundResourceInfo));
		}
		break;

		default:
			break;
		}
	}

	if constexpr (std::is_same_v<D3D1x_SHADER_DESC, D3D12_SHADER_DESC>)
	{
		if (type == ShaderType::VertexShader) {
			union {
				D3D12_SIGNATURE_PARAMETER_DESC signature_desc;
				byte signature_data[sizeof(D3D12_SIGNATURE_PARAMETER_DESC)];
			} s2d;

			size_t signature = 0;
			for (UINT i = 0; i != desc.InputParameters; ++i) {
				pReflection->GetInputParameterDesc(i, &s2d.signature_desc);
				auto seed = white::hash(s2d.signature_data);
				white::hash_combine(signature, seed);
			}

			pInfo->InputSignature = signature;
		}

		if (type == ShaderType::ComputeShader) {
			UINT x, y, z;
			pReflection->GetThreadGroupSize(&x, &y, &z);
			pInfo->CSBlockSize = white::math::vector3<white::uint16>(static_cast<white::uint16>(x),
				static_cast<white::uint16>(y), static_cast<white::uint16>(z));
		}
	}
}

void FillD3D11Reflect(ID3D11ShaderReflection* pReflection, ShaderInfo* pInfo, ShaderType type)
{
	D3D11_SHADER_DESC desc;
	pReflection->GetDesc(&desc);

	for (UINT i = 0; i != desc.ConstantBuffers; ++i) {
		auto pReflectionConstantBuffer = pReflection->GetConstantBufferByIndex(i);

		D3D11_SHADER_BUFFER_DESC buffer_desc;
		pReflectionConstantBuffer->GetDesc(&buffer_desc);
		if ((D3D_CT_CBUFFER == buffer_desc.Type) || (D3D_CT_TBUFFER == buffer_desc.Type)) {
			ShaderInfo::ConstantBufferInfo  ConstantBufferInfo;
			ConstantBufferInfo.name = buffer_desc.Name;
			ConstantBufferInfo.name_hash = white::constfn_hash(buffer_desc.Name);
			ConstantBufferInfo.size = buffer_desc.Size;
			ConstantBufferInfo.bind_point = i;

			for (UINT v = 0; v != buffer_desc.Variables; ++v) {
				auto pReflectionVar = pReflectionConstantBuffer->GetVariableByIndex(v);
				D3D11_SHADER_VARIABLE_DESC variable_desc;
				pReflectionVar->GetDesc(&variable_desc);

				D3D11_SHADER_TYPE_DESC type_desc;
				pReflectionVar->GetType()->GetDesc(&type_desc);

				ShaderInfo::ConstantBufferInfo::VariableInfo VariableInfo;
				VariableInfo.name = variable_desc.Name;
				VariableInfo.start_offset = variable_desc.StartOffset;
				VariableInfo.type = variable_desc.StartOffset;
				VariableInfo.rows = variable_desc.StartOffset;
				VariableInfo.columns = variable_desc.StartOffset;
				VariableInfo.elements = variable_desc.StartOffset;
				VariableInfo.size = variable_desc.Size;

				ConstantBufferInfo.var_desc.emplace_back(std::move(VariableInfo));
			}
			pInfo->ConstantBufferInfos.emplace_back(std::move(ConstantBufferInfo));
		}
	}

	pInfo->ResourceCounts.NumCBs = static_cast<white::uint16>(pInfo->ConstantBufferInfos.size());

	for (UINT i = 0; i != desc.BoundResources; ++i) {
		D3D11_SHADER_INPUT_BIND_DESC input_bind_desc;
		pReflection->GetResourceBindingDesc(i, &input_bind_desc);

		auto BindPoint = static_cast<white::uint16>(input_bind_desc.BindPoint + 1);

		ShaderParamClass Class = ShaderParamClass::Num;
		switch (input_bind_desc.Type)
		{
		case D3D_SIT_SAMPLER:
			pInfo->ResourceCounts.NumSamplers = std::max(pInfo->ResourceCounts.NumSamplers, BindPoint);
			Class = ShaderParamClass::Sampler;
			break;

		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			Class = ShaderParamClass::SRV;
			pInfo->ResourceCounts.NumSRVs = std::max(pInfo->ResourceCounts.NumSRVs, BindPoint);
			break;

		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			Class = ShaderParamClass::UAV;
			pInfo->ResourceCounts.NumUAVs = std::max(pInfo->ResourceCounts.NumUAVs, BindPoint);
			break;

		default:
			break;
		}

		switch (input_bind_desc.Type)
		{
		case D3D_SIT_TEXTURE:
		case D3D_SIT_SAMPLER:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		{
			ShaderInfo::BoundResourceInfo BoundResourceInfo;
			BoundResourceInfo.name = input_bind_desc.Name;
			BoundResourceInfo.type = static_cast<uint8_t>(Class);
			BoundResourceInfo.bind_point = static_cast<uint16_t>(input_bind_desc.BindPoint);
			pInfo->BoundResourceInfos.emplace_back(std::move(BoundResourceInfo));
		}
		break;

		default:
			break;
		}
	}

	if (type == ShaderType::VertexShader) {
		union {
			D3D11_SIGNATURE_PARAMETER_DESC signature_desc;
			byte signature_data[sizeof(D3D12_SIGNATURE_PARAMETER_DESC)];
		} s2d;

		size_t signature = 0;
		for (UINT i = 0; i != desc.InputParameters; ++i) {
			pReflection->GetInputParameterDesc(i, &s2d.signature_desc);
			auto seed = white::hash(s2d.signature_data);
			white::hash_combine(signature, seed);
		}

		pInfo->InputSignature = signature;
	}

	if (type == ShaderType::ComputeShader) {
		UINT x, y, z;
		pReflection->GetThreadGroupSize(&x, &y, &z);
		pInfo->CSBlockSize = white::math::vector3<white::uint16>(static_cast<white::uint16>(x),
			static_cast<white::uint16>(y), static_cast<white::uint16>(z));
	}
}

void ReportCompileResult(HRESULT hr, const char* msg)
{
	hr == S_OK ? spdlog::warn(msg) : spdlog::error(msg);
	CheckHResult(hr);
}

namespace asset::X::Shader::DXBC {
	ShaderBlob CompileToDXBC(const ShaderCompilerInput& input,
		white::uint32 flags) {
		std::vector<D3D_SHADER_MACRO> defines;
		D3D_SHADER_MACRO define_end = { nullptr, nullptr };
		defines.push_back(define_end);

		platform_ex::COMPtr<ID3DBlob> code_blob;
		platform_ex::COMPtr<ID3DBlob> error_blob;

		auto hr = D3DCompile(input.Code.data(), input.Code.size(), input.SourceName.data(), defines.data(), nullptr, input.EntryPoint.data(), CompileDXBCProfile(input.Type).data(), flags, 0, &code_blob, &error_blob);
		if (error_blob)
		{
			auto error = reinterpret_cast<char*>(error_blob->GetBufferPointer());
			ReportCompileResult(hr, error);
		}

		if (code_blob) {
			ShaderBlob blob;
			blob.first = std::make_unique<byte[]>(code_blob->GetBufferSize());
			blob.second = code_blob->GetBufferSize();
			std::memcpy(blob.first.get(), code_blob->GetBufferPointer(), blob.second);
			return std::move(blob);
		}

		return {};
	}

	void ReflectDXBC(const ShaderBlob& blob, ShaderType type, ShaderInfo* pInfo)
	{
		platform_ex::COMPtr<ID3D12ShaderReflection> pReflection;
		if (SUCCEEDED(D3DReflect(blob.first.get(), blob.second, IID_ID3D12ShaderReflection, reinterpret_cast<void**>(&pReflection.GetRef()))))
		{
			FillD3D12Reflect<ID3D12ShaderReflection, D3D12_SHADER_DESC>(pReflection.Get(), pInfo, type);
			return;
		}

		//fallback
		platform_ex::COMPtr<ID3D11ShaderReflection> pFallbackReflection;
		CheckHResult(D3DReflect(blob.first.get(), blob.second, IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&pFallbackReflection.GetRef())));
		FillD3D11Reflect(pFallbackReflection.Get(), pInfo, type);
	}

	ShaderBlob StripDXBC(const ShaderBlob& code_blob, white::uint32 flags) {
		platform_ex::COMPtr<ID3DBlob> stripped_blob;
		platform_ex::CheckHResult(D3DStripShader(code_blob.first.get(), code_blob.second, flags, &stripped_blob));
		ShaderBlob blob;
		blob.first = std::make_unique<byte[]>(stripped_blob->GetBufferSize());
		blob.second = stripped_blob->GetBufferSize();
		std::memcpy(blob.first.get(), stripped_blob->GetBufferPointer(), blob.second);
		return std::move(blob);
	}
}

// Parses ray tracing shader entry point specification string in one of the following formats:
// 1) Verbatim single entry point name, e.g. "MainRGS"
// 2) Complex entry point for ray tracing hit group shaders:
//      a) "closesthit=MainCHS"
//      b) "closesthit=MainCHS anyhit=MainAHS"
//      c) "closesthit=MainCHS anyhit=MainAHS intersection=MainIS"
//      d) "closesthit=MainCHS intersection=MainIS"
//    NOTE: closesthit attribute must always be provided for complex hit group entry points
static void ParseRayTracingEntryPoint(const std::string& Input, std::string& OutMain, std::string& OutAnyHit, std::string& OutIntersection)
{
	auto ParseEntry = [&Input](const char* Marker)
	{
		std::string Result;
		auto BeginIndex = Input.find(Marker);
		if (BeginIndex != std::string::npos)
		{
			auto EndIndex = Input.find(" ", BeginIndex);
			if (EndIndex == std::string::npos) EndIndex = Input.size() + 1;
			auto MarkerLen = std::strlen(Marker);
			auto Count = EndIndex - BeginIndex;
			Result = Input.substr(BeginIndex + MarkerLen, Count - MarkerLen);
		}
		return Result;
	};

	OutMain = ParseEntry("closesthit=");
	OutAnyHit = ParseEntry("anyhit=");
	OutIntersection = ParseEntry("intersection=");

	// If complex hit group entry is not specified, assume a single verbatim entry point
	if (OutMain.empty() && OutAnyHit.empty() && OutIntersection.empty())
	{
		OutMain = Input;
	}
}

#ifndef DXIL_FOURCC
#define DXIL_FOURCC(ch0, ch1, ch2, ch3) (                            \
  (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
  (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
  )
#endif

static dxc::DxcDllSupport& GetDxcDllHelper()
{
	static dxc::DxcDllSupport DxcDllSupport;
	static bool DxcDllInitialized = false;
	if (!DxcDllInitialized)
	{
		CheckHResult(DxcDllSupport.Initialize());
		DxcDllInitialized = true;
	}
	return DxcDllSupport;
}

template <typename T>
static HRESULT D3DCreateReflectionFromBlob(IDxcBlob* DxilBlob, COMPtr<T>& OutReflection)
{
	dxc::DxcDllSupport& DxcDllHelper = GetDxcDllHelper();

	COMPtr<IDxcContainerReflection> ContainerReflection;
	CheckHResult(DxcDllHelper.CreateInstance(CLSID_DxcContainerReflection, &ContainerReflection.GetRef()));
	CheckHResult(ContainerReflection->Load(DxilBlob));

	const white::uint32 DxilPartKind = DXIL_FOURCC('D', 'X', 'I', 'L');
	white::uint32 DxilPartIndex = ~0u;
	CheckHResult(ContainerReflection->FindFirstPartKind(DxilPartKind, &DxilPartIndex));

	HRESULT Result = ContainerReflection->GetPartReflection(DxilPartIndex, IID_PPV_ARGS(&OutReflection.GetRef()));

	return Result;
}

namespace asset::X::Shader::DXIL {
	static class CurrentDirInclude
	{
	public:
		CurrentDirInclude()
		{
			GetDxcDllHelper().CreateInstance(CLSID_DxcLibrary, &Library.GetRef());
		}

		HRESULT LoadSource(
			const std::filesystem::path& path,                                   // Candidate filename.
			IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
		)
		{
			auto key = path.string();


			try {
				auto itr = caches.find(key);
				if (itr == caches.end())
				{
					auto file = Open(path);

					auto buffer = std::make_unique<byte[]>(file.GetSize());

					auto length = file.Read(buffer.get(), file.GetSize(), 0);

					IDxcBlobEncoding* TextBlob;
					Library->CreateBlobWithEncodingOnHeapCopy(buffer.get(), static_cast<UINT32>(file.GetSize()), CP_UTF8, &TextBlob);

					itr = caches.emplace(key, TextBlob).first;
				}

				*ppIncludeSource = itr->second;
				(*ppIncludeSource)->AddRef();
				return S_OK;
			}
			catch (platform_ex::Windows::Win32Exception&)
			{
				*ppIncludeSource = nullptr;
				return E_FAIL;
			}
		}
	private:
		std::filesystem::path shaders_path = WhiteEngine::PathSet::EngineDir() / "Shaders";
		platform::File Open(const std::filesystem::path& path)
		{
			auto local_path = shaders_path / path;
			if (std::filesystem::exists(local_path))
				return platform::File(local_path.wstring(), platform::File::kToRead);
			return platform::File(path.wstring(), platform::File::kToRead);
		}

		white::ConcurrentHashMap<std::string, IDxcBlobEncoding*> caches;

		COMPtr<IDxcLibrary> Library;
	} currdir_include;

	struct IncluderHanderContext :public IDxcIncludeHandler
	{
	public:
		HRESULT LoadSource(
			LPCWSTR pFilename,                                   // Candidate filename.
			IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
		)
		{
			auto path = std::filesystem::path(pFilename);

			auto hr = currdir_include.LoadSource(path, ppIncludeSource);
			if (SUCCEEDED(hr))
			{
				auto key = path.string();

				Dependents.insert(key);
			}
			return hr;
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(
			REFIID riid,
			void** ppvObject)
		{
			return S_FALSE;
		}

		ULONG AddRef()
		{
			return 1;
		}

		ULONG Release()
		{
			return 1;
		}

		std::set<std::string> Dependents;
	};

	static void D3DCreateDXCArguments(std::vector<const WCHAR*>& OutArgs, const WCHAR* Exports, white::uint32 CompileFlags, white::uint32 AutoBindingSpace = ~0u)
	{
		// Static digit strings are used here as they are returned in OutArgs
		static const WCHAR* DigitStrings[] =
		{
			L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9"
		};

		if (AutoBindingSpace < white::size(DigitStrings))
		{
			OutArgs.push_back(L"/auto-binding-space");
			OutArgs.push_back(DigitStrings[AutoBindingSpace]);
		}
		else if (AutoBindingSpace != ~0u)
		{
			spdlog::error("Unsupported register binding space {}", AutoBindingSpace);
		}

		if (Exports && *Exports)
		{
			// Ensure that only the requested functions exists in the output DXIL.
			// All other functions and their used resources must be eliminated.
			OutArgs.push_back(L"/exports");
			OutArgs.push_back(Exports);
		}

		if (CompileFlags & D3DCOMPILE_PREFER_FLOW_CONTROL)
		{
			CompileFlags &= ~D3DCOMPILE_PREFER_FLOW_CONTROL;
			OutArgs.push_back(L"/Gfp");
		}

		if (CompileFlags & D3DCOMPILE_DEBUG)
		{
			CompileFlags &= ~D3DCOMPILE_DEBUG;
			OutArgs.push_back(L"/Zi");
			OutArgs.push_back(L"-Qembed_debug");
		}

		if (CompileFlags & D3DCOMPILE_SKIP_OPTIMIZATION)
		{
			CompileFlags &= ~D3DCOMPILE_SKIP_OPTIMIZATION;
			OutArgs.push_back(L"/Od");
		}

		if (CompileFlags & D3DCOMPILE_SKIP_VALIDATION)
		{
			CompileFlags &= ~D3DCOMPILE_SKIP_VALIDATION;
			OutArgs.push_back(L"/Vd");
		}

		if (CompileFlags & D3DCOMPILE_AVOID_FLOW_CONTROL)
		{
			CompileFlags &= ~D3DCOMPILE_AVOID_FLOW_CONTROL;
			OutArgs.push_back(L"/Gfa");
		}

		if (CompileFlags & D3DCOMPILE_PACK_MATRIX_ROW_MAJOR)
		{
			CompileFlags &= ~D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
			OutArgs.push_back(L"/Zpr");
		}

		if (CompileFlags & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY)
		{
			CompileFlags &= ~D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
			OutArgs.push_back(L"/Gec");
		}

		if (CompileFlags & D3DCOMPILE_HLSL_2021)
		{
			CompileFlags &= ~D3DCOMPILE_HLSL_2021;
			OutArgs.push_back(L"-HV 2021");
		}

		switch (CompileFlags & SHADER_OPTIMIZATION_LEVEL_MASK)
		{
		case D3DCOMPILE_OPTIMIZATION_LEVEL0:
			CompileFlags &= ~D3DCOMPILE_OPTIMIZATION_LEVEL0;
			OutArgs.push_back(L"/O0");
			break;

		case D3DCOMPILE_OPTIMIZATION_LEVEL2:
			CompileFlags &= ~D3DCOMPILE_OPTIMIZATION_LEVEL2;
			OutArgs.push_back(L"/O2");
			break;

		case D3DCOMPILE_OPTIMIZATION_LEVEL3:
			CompileFlags &= ~D3DCOMPILE_OPTIMIZATION_LEVEL3;
			OutArgs.push_back(L"/O3");
			break;
		}

		WAssert(CompileFlags == 0, "Unhandled shader compiler flag!");
	}

	static white::uint32 GetAutoBindingSpace(const ShaderType& Target)
	{
		switch (Target)
		{
		case ShaderType::RayGen:
		case ShaderType::RayMiss:
			return RAY_TRACING_REGISTER_SPACE_GLOBAL;
		case ShaderType::RayHitGroup:
		case ShaderType::RayCallable:
			return RAY_TRACING_REGISTER_SPACE_LOCAL;
		default:
			return 0;
		}
	}

	ShaderBlob CompileAndReflectDXIL(const ShaderCompilerInput& input,
		white::uint32 flags, ShaderInfo* pInfo) {
		using String = white::Text::String;

		bool bIsRayTracingShader = IsRayTracingShader(input.Type);

		std::string RayEntryPoint;
		std::string RayAnyHitEntryPoint;
		std::string RayIntersectionEntryPoint;
		std::string RayTracingExports;
		if (bIsRayTracingShader)
		{
			ParseRayTracingEntryPoint(std::string(input.EntryPoint), RayEntryPoint, RayAnyHitEntryPoint, RayIntersectionEntryPoint);

			RayTracingExports = RayEntryPoint;

			if (!RayAnyHitEntryPoint.empty())
			{
				RayTracingExports += ";";
				RayTracingExports += RayAnyHitEntryPoint;
			}

			if (!RayIntersectionEntryPoint.empty())
			{
				RayTracingExports += ";";
				RayTracingExports += RayIntersectionEntryPoint;
			}
		}


		std::vector<const wchar_t*> args;
		String wRayTracingExports(RayTracingExports);
		D3DCreateDXCArguments(args, (wchar_t*)wRayTracingExports.data(), flags, GetAutoBindingSpace(input.Type));

		dxc::DxcDllSupport& DxcDllHelper = GetDxcDllHelper();

		COMPtr<IDxcCompiler> Compiler;
		DxcDllHelper.CreateInstance(CLSID_DxcCompiler, &Compiler.GetRef());

		COMPtr<IDxcLibrary> Library;
		DxcDllHelper.CreateInstance(CLSID_DxcLibrary, &Library.GetRef());

		COMPtr<IDxcBlobEncoding> TextBlob;
		Library->CreateBlobWithEncodingFromPinned(input.Code.data(), static_cast<UINT32>(input.Code.size()), CP_UTF8, &TextBlob.GetRef());

		COMPtr<IDxcOperationResult> CompileResult;
		CheckHResult(Compiler->Compile(
			TextBlob.Get(),
			(wchar_t*)String(input.SourceName).data(),
			(wchar_t*)String(input.EntryPoint).data(),
			(wchar_t*)String(CompileDXILProfile(input.Type)).data(),
			args.data(),
			static_cast<UINT32>(args.size()),
			NULL,
			0,
			NULL,
			&CompileResult.GetRef()
		));

		HRESULT CompileResultCode = S_OK;
		CompileResult->GetStatus(&CompileResultCode);

		platform_ex::COMPtr<IDxcBlobEncoding> error_blob;
		CompileResult->GetErrorBuffer(&error_blob.GetRef());
		if (error_blob && error_blob->GetBufferSize())
		{
			BOOL Knwon = false;
			UINT32 CodePage = CP_UTF8;
			if (SUCCEEDED(error_blob->GetEncoding(&Knwon, &CodePage)))
			{
				auto error = platform_ex::Windows::MBCSToMBCS(std::string_view((const char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize()),
					CodePage, CP_ACP);
				white::replace_all(error, "hlsl.hlsl", input.SourceName);
				ReportCompileResult(CompileResultCode, error.c_str());
			}
			else {
				ReportCompileResult(CompileResultCode, (const char*)error_blob->GetBufferPointer());
			}
		}

		platform_ex::COMPtr<IDxcBlob> code_blob;
		CheckHResult(CompileResult->GetResult(&code_blob.GetRef()));

		if (pInfo)
		{
			wconstraint(code_blob.Get());
			if (bIsRayTracingShader)
			{
				COMPtr<ID3D12LibraryReflection> LibraryReflection;

				HRESULT Result = D3DCreateReflectionFromBlob(code_blob.Get(), LibraryReflection);

				CheckHResult(Result);

				D3D12_LIBRARY_DESC LibraryDesc = {};
				LibraryReflection->GetDesc(&LibraryDesc);

				//?QualifiedName@ (as described here: https://en.wikipedia.org/wiki/Name_mangling)
				// Entry point parameters are currently not included in the partial mangling.
				std::vector<std::string> MangledEntryPoints;
				if (!RayEntryPoint.empty())
				{
					MangledEntryPoints.push_back(white::sfmt("?%s@", RayEntryPoint.c_str()));
				}
				if (!RayAnyHitEntryPoint.empty())
				{
					MangledEntryPoints.push_back(white::sfmt("?%s@", RayAnyHitEntryPoint.c_str()));
				}
				if (!RayIntersectionEntryPoint.empty())
				{
					MangledEntryPoints.push_back(white::sfmt("?%s@", RayIntersectionEntryPoint.c_str()));
				}

				//merges the reflection data for multiple functions 
				white::uint32 NumFoundEntryPoints = 0;

				for (white::uint32 FunctionIndex = 0; FunctionIndex < LibraryDesc.FunctionCount; ++FunctionIndex)
				{
					auto FunctionReflection = LibraryReflection->GetFunctionByIndex(FunctionIndex);

					D3D12_FUNCTION_DESC FunctionDesc = {};
					FunctionReflection->GetDesc(&FunctionDesc);

					for (auto& MangledEntryPoint : MangledEntryPoints)
					{
						if (strstr(FunctionDesc.Name, MangledEntryPoint.c_str()))
						{
							FillD3D12Reflect<ID3D12FunctionReflection, D3D12_FUNCTION_DESC>(FunctionReflection, pInfo, input.Type);

							++NumFoundEntryPoints;
						}
					}
				}

				RayTracingShaderInfo info;
				info.EntryPoint = RayEntryPoint;
				info.AnyHitEntryPoint = RayAnyHitEntryPoint;
				info.IntersectionEntryPoint = RayIntersectionEntryPoint;

				pInfo->RayTracingInfos = info;
			}
			else
			{
				COMPtr<ID3D12ShaderReflection> ShaderReflection;

				HRESULT Result = D3DCreateReflectionFromBlob(code_blob.Get(), ShaderReflection);

				CheckHResult(Result);

				FillD3D12Reflect<ID3D12ShaderReflection, D3D12_SHADER_DESC>(ShaderReflection.Get(), pInfo, input.Type);
			}
		}

		ShaderBlob blob;
		blob.first = std::make_unique<byte[]>(code_blob->GetBufferSize());
		blob.second = code_blob->GetBufferSize();
		std::memcpy(blob.first.get(), code_blob->GetBufferPointer(), blob.second);
		return std::move(blob);
	}

	ShaderBlob StripDXIL(const ShaderBlob& code_blob, white::uint32 flags) {
		ShaderBlob blob;
		blob.first = std::make_unique<byte[]>(code_blob.second);
		blob.second = code_blob.second;
		std::memcpy(blob.first.get(), code_blob.first.get(), blob.second);
		return std::move(blob);
	}

	PreprocessOutput PreprocessShader(const std::string& code, const ShaderCompilerInput& input)
	{
		using String = white::Text::String;

		std::vector<const wchar_t*> args;

		std::vector<std::pair<String, String>> def_holder;
		for (auto& macro : input.Environment.GetDefinitions()) {
			def_holder.emplace_back(String(macro.first.c_str(), macro.first.size()), String(macro.second.c_str(), macro.second.size()));
		}

		std::vector<DxcDefine> defs;
		for (auto& def : def_holder)
		{
			DxcDefine define;
			define.Name = (wchar_t*)def.first.c_str();
			define.Value = (wchar_t*)def.second.c_str();
			defs.emplace_back(define);
		}

		dxc::DxcDllSupport& DxcDllHelper = GetDxcDllHelper();

		COMPtr<IDxcCompiler> Compiler;
		DxcDllHelper.CreateInstance(CLSID_DxcCompiler, &Compiler.GetRef());

		COMPtr<IDxcLibrary> Library;
		DxcDllHelper.CreateInstance(CLSID_DxcLibrary, &Library.GetRef());

		COMPtr<IDxcBlobEncoding> TextBlob;
		Library->CreateBlobWithEncodingFromPinned(code.c_str(), static_cast<UINT32>(code.size()), CP_UTF8, &TextBlob.GetRef());

		IncluderHanderContext include_handler;
		COMPtr<IDxcOperationResult> CompileResult;
		CheckHResult(Compiler->Preprocess(
			TextBlob.Get(),
			NULL,
			args.data(),
			static_cast<UINT32>(args.size()),
			defs.data(),
			static_cast<UINT32>(defs.size()),
			&include_handler,
			&CompileResult.GetRef()
		));

		HRESULT CompileResultCode = S_OK;
		CompileResult->GetStatus(&CompileResultCode);

		platform_ex::COMPtr<IDxcBlobEncoding> error_blob;
		CompileResult->GetErrorBuffer(&error_blob.GetRef());
		if (error_blob && error_blob->GetBufferSize())
		{
			BOOL Knwon = false;
			UINT32 CodePage = CP_UTF8;
			if (SUCCEEDED(error_blob->GetEncoding(&Knwon, &CodePage)))
			{
				auto error = platform_ex::Windows::MBCSToMBCS(std::string_view((const char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize()),
					CodePage, CP_ACP);
				white::replace_all(error, "hlsl.hlsl", input.SourceName);
				ReportCompileResult(CompileResultCode, error.c_str());
			}
			else {
				ReportCompileResult(CompileResultCode, (const char*)error_blob->GetBufferPointer());
			}
		}

		platform_ex::COMPtr<IDxcBlob> code_blob;
		CheckHResult(CompileResult->GetResult(&code_blob.GetRef()));

		platform_ex::COMPtr<IDxcBlobEncoding> code_utf_blob;
		CheckHResult(Library->GetBlobAsUtf8(code_blob.Get(), &code_utf_blob.GetRef()));

		std::string preprocess_code{ (const char*)code_utf_blob->GetBufferPointer(), code_utf_blob->GetBufferSize() };
		white::replace_all(preprocess_code, "hlsl.hlsl", input.SourceName);

		return
		{
			.Code = preprocess_code,
			.Dependent = include_handler.Dependents
		};
	}
}

namespace asset::X::Shader
{
	ShaderBlob CompileAndReflect(const ShaderCompilerInput& input,
		white::uint32 flags, ShaderInfo* pInfo)
	{
		bool use_dxc = IsRayTracingShader(input.Type);

		if (flags & D3DCOMPILE_HLSL_2021)
			use_dxc = true;

		bool use_dxbc = !use_dxc;

		LOG_TRACE("CompileAndReflect {}- Entry:{} ", input.SourceName, input.EntryPoint);

		if (use_dxbc)
		{
			auto blob = DXBC::CompileToDXBC(input, flags);

			if (pInfo)
				DXBC::ReflectDXBC(blob, input.Type, pInfo);

			return blob;
		}

		return DXIL::CompileAndReflectDXIL(input, flags, pInfo);
	}

	ShaderBlob Strip(const ShaderBlob& code_blob, ShaderType type, white::uint32 flags) {
		bool use_dxc = IsRayTracingShader(type);

		auto strip_ptr = use_dxc ? DXIL::StripDXIL : DXBC::StripDXBC;

		return (*strip_ptr)(code_blob, flags);
	}
}

#else
//TODO CryEngine HLSLCross Compiler?
//Other Target Platfom Compiler [Tool...]
#endif