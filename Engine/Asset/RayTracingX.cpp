#include "RayTracingX.h"
#include "ShaderAsset.h"
#include "D3DShaderCompiler.h"
#include "Runtime/AssetResourceScheduler.h"
#include "WFramework/Helper/ShellHelper.h"
#include "ShaderLoadingDesc.h"
#include <unordered_map>

using namespace platform;
using namespace asset;
using namespace platform::Render::Shader;
using namespace white;

struct RayTracingShaderAsset :public asset::ShadersAsset,public asset::AssetName
{
	std::unordered_map<ShaderType, std::string> EntryPoints;
	std::unordered_map< ShaderType, ShaderCode> RayCodes;
};

class RayTracingShaderLoadingDesc : public asset::AssetLoading<RayTracingShaderAsset>, public platform::X::ShaderLoadingDesc<RayTracingShaderAsset> {
private:
	using Super = platform::X::ShaderLoadingDesc<RayTracingShaderAsset>;
public:
	explicit RayTracingShaderLoadingDesc(platform::X::path const& shaderpath)
		:Super(shaderpath)
	{
	}

	std::size_t Type() const override {
		return white::type_id<RayTracingShaderLoadingDesc>().hash_code();
	}

	std::size_t Hash() const override {
		return white::hash_combine_seq(Type(), Super::Hash());
	}

	const asset::path& Path() const override {
		return Super::Path();
	}

	white::coroutine::Task<std::shared_ptr<AssetType>> GetAwaiter() override {
		PreCreate();
		LoadNode();
		ParseNode();
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
		ParseEntryPoints();
		return nullptr;
	}

	std::shared_ptr<AssetType> CreateAsset()
	{
		Compile();
		return ReturnValue();
	}

	void ParseEntryPoints()
	{
		auto& node = GetNode();
		if (auto pRayGen = AccessPtr("raygen_shader", node))
		{
			GetAsset()->EntryPoints.emplace(ShaderType::RayGen, *pRayGen);
		}
		if (auto pRayMiss = AccessPtr("raymiss_shader", node))
		{
			GetAsset()->EntryPoints.emplace(ShaderType::RayMiss, *pRayMiss);
		}
		if (auto pRayHitGroup = AccessPtr("rayhitgroup_shader", node))
		{
			GetAsset()->EntryPoints.emplace(ShaderType::RayHitGroup, scheme::Deliteralize(*pRayHitGroup));
		}
		if (auto pRayCallable = AccessPtr("raycallable_shader", node))
		{
			GetAsset()->EntryPoints.emplace(ShaderType::RayCallable, *pRayCallable);
		}
	}

	void Compile()
	{
		auto path = Path().string();
#ifndef NDEBUG
		path += ".hlsl";
		{
			std::ofstream fout(path);
			fout << GetCode();
		}
#endif

		asset::X::Shader::ShaderCompilerInput input;
		input.Code = GetCode();
		input.SourceName = path;

		std::vector<ShaderMacro> macros{ GetAsset()->GetMacros() };
		for (auto& pair : GetAsset()->EntryPoints)
		{
			input.Type = pair.first;
			input.EntryPoint = pair.second;

			asset::X::Shader::AppendCompilerEnvironment(input.Environment, input.Type);

			auto pInfo = std::make_unique<ShaderInfo>(input.Type);

			LOG_TRACE("CompileAndReflect Type:{} ", input.EntryPoint);
			auto blob =asset::X::Shader::CompileAndReflect(input,
#ifndef NDEBUG
				D3DFlags::D3DCOMPILE_DEBUG
#else
				D3DFlags::D3DCOMPILE_OPTIMIZATION_LEVEL3
#endif
				, pInfo.get()
			);

			ShaderInitializer initializer;
			initializer.pBlob = &blob;
			initializer.pInfo = pInfo.get();

			ShaderCompilerOutput Output;
			GenerateOuput(initializer, Output);
			GetAsset()->RayCodes.emplace(pair.first, Output.ShaderCode);
		}
	}
};

#include "RenderInterface/IContext.h"
#include "RenderInterface/IRayContext.h"

using namespace platform::Render;

std::shared_ptr<Render::RayTracingShader> platform::X::LoadRayTracingShader(Render::RayDevice& Device, const path& filepath)
{
	auto pAsset = AssetResourceScheduler::Instance().SyncLoad<RayTracingShaderLoadingDesc>(filepath);

	auto& RayDevice = Context::Instance().GetRayContext().GetDevice();

	for (auto& pair : pAsset->RayCodes)
	{
		std::vector<uint8> UnCompressCode;
		auto ShaderCode = TryUncompressCode(pair.second.GetReadAccess(), pair.second.GetUncompressedSize(), UnCompressCode);

		return shared_raw_robject(RayDevice.CreateRayTracingShader(white::make_const_span(ShaderCode, pair.second.GetUncompressedSize())));
	}

	return std::shared_ptr<Render::RayTracingShader>();
}
