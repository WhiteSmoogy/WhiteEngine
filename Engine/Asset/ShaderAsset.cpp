#include "ShaderAsset.h"
#include "System/SystemEnvironment.h"
#include "WFramework/Helper/ShellHelper.h"
#include "Runtime/Path.h"

using namespace asset;
using namespace platform::Render::Shader;

void asset::AssetName::SetName(const std::string& Name)
{
	name = Name;
	hash = white::constfn_hash(name);
}

constexpr int MaxSpace = 4;

class ResourceRegisterSlotAllocator
{
public:
	enum ResourceType
	{
		t,// for shader resource views (SRV)
		s,// for samplers
		u,//for unordered access views (UAV)
		b //for constant buffer views (CBV)
	};

	struct SpaceRegisters
	{
		std::unordered_map<ResourceType, std::vector<bool>> registers;


		void SetUse(std::vector<bool>& c, int index)
		{
			c.resize(index + 1);
			c[index] = true;
		}

		void SetUse(ResourceType type, int index)
		{
			SetUse(registers[type], index);
		}

		int FindEmpty(std::vector<bool>& c)
		{
			for (int i = 0; i != c.size(); ++i)
			{
				if (!c[i])
				{
					c[i] = true;
					return i;
				}
			}

			auto index = c.size();

			c.resize(c.size() + 1);
			c[index] = true;
			return static_cast<int>(index);
		}

		int FindEmpty(ResourceType type)
		{
			return FindEmpty(registers[type]);
		}
	};

	SpaceRegisters Spaces[MaxSpace];

	std::pair<ResourceType, int> AllocatorIndex(const BindDesc& desc, ShaderParamType type)
	{
		auto space = desc.GetSpace() == BindDesc::Any ? 0 : desc.GetSpace();

		wconstraint(space < MaxSpace);

		auto register_type = to_type(type);

		if (desc.GetIndex() == BindDesc::Any)
		{
			return { register_type,Spaces[space].FindEmpty(register_type) };
		}
		else
		{
			Spaces[space].SetUse(register_type, desc.GetIndex());

			return { register_type,desc.GetIndex() };
		}
	}

	void PreAllocator(const BindDesc& desc, ShaderParamType type)
	{
		auto space = desc.GetSpace() == BindDesc::Any ? 0 : desc.GetSpace();

		wconstraint(space < MaxSpace);

		auto register_type = to_type(type);

		if (desc.GetIndex() != BindDesc::Any)
		{
			Spaces[space].SetUse(register_type, desc.GetIndex());
		}
	}

	static char to_char(ResourceType type)
	{
		switch (type)
		{
		case ResourceRegisterSlotAllocator::t:
			return 't';
			break;
		case ResourceRegisterSlotAllocator::s:
			return 's';
			break;
		case ResourceRegisterSlotAllocator::u:
			return 'u';
			break;
		case ResourceRegisterSlotAllocator::b:
			return 'b';
			break;
		default:
			wconstraint(false);
		}
	}

	static ResourceType to_type(ShaderParamType type)
	{
		if (type == SPT_ConstantBuffer)
			return b;
		if (type == SPT_sampler)
			return s;
		switch (type)
		{
		case SPT_rwbuffer:
		case SPT_rwstructured_buffer:
		case SPT_rwtexture1D:
		case SPT_rwtexture2D:
		case SPT_rwtexture3D:
		case SPT_rwtexture1DArray:
		case SPT_rwtexture2DArray:
		case SPT_AppendStructuredBuffer:
		case SPT_rwbyteAddressBuffer:
			return u;
		default:
			return t;
		}
	}
};

std::string FormatBindDesc(ResourceRegisterSlotAllocator& allocaotr, const BindDesc& desc, ShaderParamType type)
{
	if (desc.GetIndex() == BindDesc::Any && desc.GetSpace() == BindDesc::Any)
		return {};

	auto type_index = allocaotr.AllocatorIndex(desc, type);

	auto empty_space = desc.GetSpace() == BindDesc::Any;

	std::string hlsl = ":register(";

	hlsl += ResourceRegisterSlotAllocator::to_char(type_index.first);

	hlsl += std::to_string(type_index.second);

	if (!empty_space)
	{
		hlsl += ",space";
		hlsl += std::to_string(desc.GetSpace());
	}

	hlsl += ")";

	return hlsl;
}

bool asset::RequireStructElemType(ShaderParamType type)
{
	switch (type)
	{
	case SPT_AppendStructuredBuffer:
	case SPT_ConsumeStructuredBuffer:
	case SPT_StructuredBuffer:
	case SPT_rwstructured_buffer:
	case SPT_ConstantBuffer:
		return true;
	}

	return false;
}


bool asset::RequireElemType(ShaderParamType type)
{
	if (GetBaseType(type) == SBT_UAV && type != SPT_rwbyteAddressBuffer)
		return true;

	if (RequireStructElemType(type))
		return true;

	if (type == SPT_buffer || type == SPT_rwbuffer)
		return true;

	return false;
}

std::string asset::ShadersAsset::GenHLSLShader() const
{
	using std::endl;

	auto& macros = GetMacros();
	auto& cbuffers = GetCBuffers();
	auto& params = GetParams();
	auto& fragments = GetFragments();

	ResourceRegisterSlotAllocator allocator;
	//pre allocator
	for (auto& gen_index : gen_indices) {
		auto local_index = std::get<1>(gen_index);
		switch (std::get<0>(gen_index)) {
			case	CBUFFER:
			{
				auto& cbuffer = cbuffers[local_index];
				allocator.PreAllocator(cbuffer, SPT_ConstantBuffer);
			}
			case	PARAM:
			{
				auto& param = params[local_index];
				allocator.PreAllocator(param, param.GetType());
			}
		}
	}

	std::stringstream ss;

	for (auto& gen_index : gen_indices) {
		auto local_index = std::get<1>(gen_index);
		switch (std::get<0>(gen_index)) {
		case	MACRO:
		{
			auto& name_value = macros[local_index];
			ss << white::sfmt("#define %s %s", name_value.first.c_str(), name_value.second.c_str()) << endl;
			break;
		}
		case	CBUFFER:
		{
			auto& cbuffer = cbuffers[local_index];

			bool template_synatx = !cbuffer.GetElemInfo().empty();

			wconstraint(!template_synatx || !cbuffer.GetElemInfo().empty());

			if (!template_synatx)
				ss << white::sfmt("cbuffer %s", cbuffer.GetName().c_str()) << endl;
			else
				ss << white::sfmt("ConstantBuffer<%s> %s", cbuffer.GetElemInfo().c_str(), cbuffer.GetName().c_str()) << endl;

			ss << FormatBindDesc(allocator, cbuffer, SPT_ConstantBuffer);

			if (!template_synatx)
			{
				ss << '{' << endl;

				for (auto paramindex : cbuffer.GetParamIndices())
				{
					const auto& param = GetParams()[paramindex];
					if (param.GetType() >= SPT_bool) {
						ss << white::sfmt("%s %s", GetTypeName(param.GetType()).c_str(), param.GetName().c_str());
						if (param.GetArraySize())
							ss << white::sfmt("[%s]", std::to_string(param.GetArraySize()).c_str());
						ss << ';' << endl;
					}
				}

				ss << "};" << endl;
			}
			else
			{
				ss << ";" << endl;
			}
			break;
		}
		case	PARAM:
		{
			auto& param = params[local_index];
			std::string elem_type = [&] {
				if (auto pElemType = std::get_if<ShaderParamType>(&param.GetElemInfo())) {
					if (*pElemType != SPT_ElemEmpty)
						return GetTypeName(param.GetElemType());
					else
						return std::string();
				}
				else {
					return param.GetElemUserType();
				}
			}();

			wconstraint(!RequireElemType(param.GetType())|| !elem_type.empty());

			if (IsElemType(param.GetType()) && !elem_type.empty()) {
				ss << white::sfmt("%s<%s> %s", GetTypeName(param.GetType()).c_str(),
					elem_type.c_str(), param.GetName().c_str())
					<< FormatBindDesc(allocator, param, param.GetType())
					<< ';'
					<< endl;
			}
			else {
				ss << white::sfmt("%s %s", GetTypeName(param.GetType()).c_str(),
					param.GetName().c_str())
					<< FormatBindDesc(allocator, param, param.GetType())
					<< ';'
					<< endl;
			}
			break;
		}
		case	FRAGMENT:
		{
			auto& fragment = fragments[local_index];
			ss << fragment.GetFragment() << endl;
			break;
		}
		}
	}

	return ss.str();
}


template<>
std::optional<platform::Render::ShaderInfo::ConstantBufferInfo> asset::ShadersAsset::GetInfo<platform::Render::ShaderInfo::ConstantBufferInfo>(const std::string_view& name) const {
	using platform::Render::ShaderCompose;
	using platform::Render::ShaderInfo;
	auto hash = white::constfn_hash(name);
	for (auto& pair : blobs) {
		auto& blob = pair.second;
		auto& Info = blob.GetInfo();

		for (auto& info : Info.ConstantBufferInfos) {
			if (info.name_hash == hash)
				return info;
		}
	}
	return {};
}

template<>
std::optional<platform::Render::ShaderInfo::ConstantBufferInfo::VariableInfo> asset::ShadersAsset::GetInfo<platform::Render::ShaderInfo::ConstantBufferInfo::VariableInfo>(const std::string_view& name) const {
	using platform::Render::ShaderCompose;
	using platform::Render::ShaderInfo;
	auto hash = white::constfn_hash(name);
	for (auto& pair : blobs) {
		auto& blob = pair.second;
		auto& Info = blob.GetInfo();

		for (auto& info : Info.ConstantBufferInfos) {
			for (auto& varinfo : info.var_desc) {
				if (varinfo.name == name)
					return varinfo;
			}
		}
	}
	return {};
}

template std::optional<platform::Render::ShaderInfo::ConstantBufferInfo> asset::ShadersAsset::GetInfo(const std::string_view& name) const;
template std::optional<platform::Render::ShaderInfo::ConstantBufferInfo::VariableInfo> asset::ShadersAsset::GetInfo(const std::string_view& name) const;

class type_define
{
public:
	static type_define& Instance()
	{
		static type_define instance;
		return instance;
	}

	uint32_t type_code(std::string const& name) const
	{
		auto lowername = name;
		white::to_lower(lowername);
		auto const name_hash = white::constfn_hash(lowername);
		for (uint32_t i = 0; i < hashs.size(); ++i)
		{
			if (hashs[i] == name_hash)
			{
				return types[i].second;
			}
		}
		throw white::unsupported();
	}

	std::string const& type_name(uint32_t code) const
	{
		for (uint32_t i = 0; i < types.size(); ++i)
		{
			if (types[i].second == code)
			{
				return types[i].first;
			}
		}
		throw white::unsupported();
	}

	type_define()
	{
		types.emplace_back("Texture1D",SPT_texture1D);
		types.emplace_back("Texture2D", SPT_texture2D);
		types.emplace_back("Texture3D", SPT_texture3D);
		types.emplace_back("TextureCUBE", SPT_textureCUBE);
		types.emplace_back("Texture1DArray", SPT_texture1DArray);
		types.emplace_back("Texture2DArray", SPT_texture2DArray);
		types.emplace_back("Texture3DArray", SPT_texture3DArray);
		types.emplace_back("TextureCUBEArray", SPT_textureCUBEArray);
		types.emplace_back("ConstantBuffer", SPT_ConstantBuffer);
		types.emplace_back("Buffer", SPT_buffer);
		types.emplace_back("StructuredBuffer", SPT_StructuredBuffer);
		types.emplace_back("RWBuffer", SPT_rwbuffer);
		types.emplace_back("RWStructuredBuffer", SPT_rwstructured_buffer);
		types.emplace_back("RWTexture1D", SPT_rwtexture1D);
		types.emplace_back("RWTexture2D", SPT_rwtexture2D);
		types.emplace_back("RWTexture3D", SPT_rwtexture3D);
		types.emplace_back("RWTexture1DArray", SPT_rwtexture1DArray);
		types.emplace_back("RWTexture2DArray", SPT_rwtexture2DArray);
		types.emplace_back("AppendStructuredBuffer", SPT_AppendStructuredBuffer);
		types.emplace_back("ConsumeStructuredBuffer", SPT_ConsumeStructuredBuffer);
		types.emplace_back("ByteAddressBuffer", SPT_byteAddressBuffer);
		types.emplace_back("RWByteAddressBuffer", SPT_rwbyteAddressBuffer);
		types.emplace_back("RaytracingAccelerationStructure", SPT_RaytracingAccelerationStructure);
		types.emplace_back("sampler",  SPT_sampler);
		types.emplace_back("shader",   SPT_shader);
		types.emplace_back("bool",     SPT_bool);
		types.emplace_back("string",   SPT_string);
		types.emplace_back("uint",     SPT_uint);
		types.emplace_back("uint2",    SPT_uint2);
		types.emplace_back("uint3",    SPT_uint3);
		types.emplace_back("uint4",    SPT_uint4);
		types.emplace_back("int",      SPT_int);
		types.emplace_back("int2",     SPT_int2);
		types.emplace_back("int3",     SPT_int3);
		types.emplace_back("int4",	   SPT_int4);
		types.emplace_back("float",    SPT_float);
		types.emplace_back("float2",   SPT_float2);
		types.emplace_back("float2x2", SPT_float2x2);
		types.emplace_back("float2x3", SPT_float2x3);
		types.emplace_back("float2x4", SPT_float2x4);
		types.emplace_back("float3",   SPT_float3);
		types.emplace_back("float3x2", SPT_float3x2);
		types.emplace_back("float3x3", SPT_float3x3);
		types.emplace_back("float3x4", SPT_float3x4);
		types.emplace_back("float4",   SPT_float4);
		types.emplace_back("float4x2", SPT_float4x2);
		types.emplace_back("float4x3", SPT_float4x3);
		types.emplace_back("float4x4", SPT_float4x4);

		for (auto type : types) {
			white::to_lower(type.first);
			hashs.emplace_back(white::constfn_hash(type.first));
		}
	}

private:
	std::vector<std::pair<std::string, ShaderParamType>> types;
	std::vector<size_t> hashs;
};




std::string asset::ShadersAsset::GetTypeName(ShaderParamType type) {
	return type_define::Instance().type_name(type);
}

ShaderParamType asset::ShadersAsset::GetType(const std::string& name)
{
	return (ShaderParamType)type_define::Instance().type_code(name);
}

#include "Runtime/AssetResourceScheduler.h"
#include "WFramework/Helper/ShellHelper.h"
#include "ShaderLoadingDesc.h"
using namespace platform::X;

using namespace platform;
using namespace asset;
using namespace platform::Render::Shader;
using namespace white;

struct HLSLAsset :public asset::ShadersAsset, public asset::AssetName
{
	std::string Code;
};

class HLSLLoadingDesc : public asset::AssetLoading<HLSLAsset>, public platform::X::ShaderLoadingDesc<HLSLAsset> {
private:
	using Super = platform::X::ShaderLoadingDesc<HLSLAsset>;
public:
	explicit HLSLLoadingDesc(platform::X::path const& shaderpath)
		:Super(shaderpath)
	{
	}

	std::size_t Type() const override {
		return white::type_id<HLSLLoadingDesc>().hash_code();
	}

	std::size_t Hash() const override {
		return white::hash_combine_seq(Type(), Super::Hash());
	}

	const asset::path& Path() const override {
		return Super::Path();
	}

	asset::path Path(const asset::path& path) const override
	{
		static std::filesystem::path shaders_path = WhiteEngine::PathSet::EngineDir() / "Shaders";

		auto local_path = shaders_path / path;
		if (std::filesystem::exists(local_path))
			return local_path;

		return path;
	}

	white::coroutine::Task<std::shared_ptr<HLSLAsset>> GetAwaiter()
	{
		if (Path().extension() == ".hlsl")
		{
			shader_desc.asset = std::make_shared<AssetType>();

			auto file = white::coroutine::ReadOnlyFile::open(Environment->Scheduler->GetIOScheduler(), Path(Path()).string());

			auto buffer = co_await LoadFileAsync(file);

			GetAsset()->Code = std::string(reinterpret_cast<const char*>(buffer.data()),buffer.size());
			co_return ReturnValue();
		}

		PreCreate();

		{
			LOG_TRACE("LoadNode {}", Path().string());
			co_await LoadNodeAsync(Environment->Scheduler->GetIOScheduler());
		}
		{
			LOG_TRACE("ParseNode {}", Path().string());
			co_await ParseNodeAsync(Environment->Scheduler->GetIOScheduler());
		}

		co_await Environment->Scheduler->schedule();

		co_return CreateAsset();
	}
private:
	std::shared_ptr<AssetType> PreCreate()
	{
		Super::PreCreate();
		return nullptr;
	}

	std::shared_ptr<AssetType> LoadNode()
	{
		Super::LoadNode();
		return  nullptr;
	}

	std::shared_ptr<AssetType> ParseNode()
	{
		Super::ParseNode();
		return nullptr;
	}

	std::shared_ptr<AssetType> CreateAsset()
	{
		GetAsset()->Code = GetCode();
		return ReturnValue();
	}
};


std::string platform::X::GenHlslShader(const path& filepath)
{
	auto pAsset = AssetResourceScheduler::Instance().SyncLoad<HLSLLoadingDesc>(filepath);

	return pAsset->Code;;
}

white::coroutine::Task<std::string> platform::X::GenHlslShaderAsync(const path& filepath)
{
	auto pAsset = co_await  AssetResourceScheduler::Instance().AsyncLoad<HLSLLoadingDesc>(filepath);

	co_return pAsset->Code;
}