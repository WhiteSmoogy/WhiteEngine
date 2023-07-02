#include "WFramework/Win32/WCLib/COM.h"
#include "WFramework/Helper/ShellHelper.h"
#include<WBase/typeinfo.h>

#include "ShaderLoadingDesc.h"
#include <WBase/id.hpp>

#include <WScheme/WScheme.h>


#include "RenderInterface/IContext.h"
#include "Runtime/Core/AssetResourceScheduler.h"

#include "EffectX.h"
#include "WSLAssetX.h"
#include "D3DShaderCompiler.h"
#include "Runtime/Core/Path.h"

#pragma warning(disable:4715) //return value or throw exception;
using namespace platform::Render::Shader;
using namespace asset::X::Shader;

namespace platform {
	using namespace scheme;

	class EffectLoadingDesc : public asset::AssetLoading<asset::EffectAsset>,public X::ShaderLoadingDesc<asset::EffectAsset> {
	private:
		using Super = X::ShaderLoadingDesc<asset::EffectAsset>;
	public:
		explicit EffectLoadingDesc(X::path const & effectpath)
			:Super(effectpath)
		{
		}

		std::size_t Type() const override {
			return white::type_id<EffectLoadingDesc>().hash_code();
		}

		std::size_t Hash() const override {
			return white::hash_combine_seq(Type(),Super::Hash());
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
		void PreCreate()
		{
			Super::PreCreate();
		}

		asset::path Path(const asset::path& path) const override
		{
			static std::filesystem::path effect_path = WhiteEngine::PathSet::EngineDir() / "RenderInterface/Effect";

			auto local_path = effect_path / path;
			if (std::filesystem::exists(local_path))
				return local_path;

			return path;
		}

		void LoadNode()
		{
			Super::LoadNode();
		}

		void ParseNode()
		{
			Super::ParseNode();
			ParseTechnique(GetNode());
		}

		std::shared_ptr<AssetType> CreateAsset()
		{
			return ReturnValue();
		}

		void ParseTechnique(const scheme::TermNode& effect_node)
		{
			auto techniques = X::SelectNodes("technique", effect_node);
			for (auto & technique_node : techniques) {
				asset::EffectTechniqueAsset technique;
				technique.SetName(Access("name", technique_node));
				auto inherit = AccessPtr("inherit", technique_node);
				if (inherit) {
					auto exist_techniques = GetAsset()->GetTechniques();
					auto inherit_technique = std::find_if(exist_techniques.begin(), exist_techniques.end(), [&inherit](const asset::EffectTechniqueAsset& exist)
					{
						return exist.GetName() == *inherit;
					});
					WAssert(inherit_technique != exist_techniques.end(), "Can't Find Inherit Technique");
					technique.GetMacrosRef() = inherit_technique->GetMacros();
					technique.GetPassesRef() = inherit_technique->GetPasses();
				}
				ParseMacro(technique.GetMacrosRef(), technique_node,true);

				auto passes = X::SelectNodes("pass", technique_node);
				for (auto & pass_node : passes) {
					asset::TechniquePassAsset pass;
					pass.SetName(Access("name", pass_node));

					asset::TechniquePassAsset* pass_ptr = nullptr;
					if (inherit) {
						auto& exist_passes = technique.GetPassesRef();
						auto exist_pass = std::find_if(exist_passes.begin(), exist_passes.end(), [&pass](const asset::TechniquePassAsset& exist)
						{
							return pass.GetNameHash() == exist.GetNameHash();
						}
						);
						if (exist_pass != exist_passes.end())
							pass_ptr = &(*exist_pass);
					}
					if (!pass_ptr)
					{
						technique.GetPassesRef().emplace_back();
						pass_ptr = &technique.GetPassesRef().back();
						pass_ptr->SetName(pass.GetName());
					}

					auto inherit_pass = AccessPtr("inherit", pass_node);
					if (inherit_pass) {
						auto exist_passes = technique.GetPasses();
						auto exist_pass = std::find_if(exist_passes.begin(), exist_passes.end(), [&inherit_pass](const asset::TechniquePassAsset& exist)
						{
							return *inherit_pass == exist.GetName();
						}
						);
						WAssert(exist_pass != exist_passes.end(), "Can't Find Inherit Technique");
						pass_ptr->GetMacrosRef() = exist_pass->GetMacros();
						pass_ptr->GetPipleStateRef() = exist_pass->GetPipleState();
					}

					ParseMacro(pass.GetMacrosRef(), pass_node,true);
					ParsePassState(*pass_ptr, pass_node);
					ComposePassShader(technique, *pass_ptr, pass_node);
				}
				GetAsset()->GetTechniquesRef().emplace_back(std::move(technique));
			}
		}

		void ParsePassState(asset::TechniquePassAsset& pass, scheme::TermNode& pass_node)
		{
			using white::constfn_hash;
			using namespace Render;
			auto to_bool = [](const std::string& value) {
				return value.size() > 0 && value[0] == 't';
			};

			

			auto to_uint16 = [](const std::string& value) {
				return static_cast<white::uint16>(std::stoul(value));
			};

			auto to_uint32 = [](const std::string& value) {
				return static_cast<white::uint32>(std::stoul(value));
			};


			auto & rs_desc = pass.GetPipleStateRef().RasterizerState;
			auto & ds_desc = pass.GetPipleStateRef().DepthStencilState;
			auto & bs_desc = pass.GetPipleStateRef().BlendState;

			auto default_parse = [&](std::size_t hash, const std::string& value) {
				switch (hash)
				{
				case constfn_hash("blend_enable_0"):
					bs_desc.blend_enable[0] = to_bool(value);
					break;
				case constfn_hash("blend_enable_1"):
					bs_desc.blend_enable[1] = to_bool(value);
					break;
				case constfn_hash("blend_enable_2"):
					bs_desc.blend_enable[2] = to_bool(value);
					break;
				case constfn_hash("blend_enable_3"):
					bs_desc.blend_enable[3] = to_bool(value);
					break;
				case constfn_hash("blend_enable_4"):
					bs_desc.blend_enable[4] = to_bool(value);
					break;
				case constfn_hash("blend_enable_5"):
					bs_desc.blend_enable[5] = to_bool(value);
					break;
				case constfn_hash("blend_enable_6"):
					bs_desc.blend_enable[6] = to_bool(value);
					break;
				case constfn_hash("blend_enable_7"):
					bs_desc.blend_enable[7] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_0"):
					bs_desc.logic_op_enable[0] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_1"):
					bs_desc.logic_op_enable[1] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_2"):
					bs_desc.logic_op_enable[2] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_3"):
					bs_desc.logic_op_enable[3] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_4"):
					bs_desc.logic_op_enable[4] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_5"):
					bs_desc.logic_op_enable[5] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_6"):
					bs_desc.logic_op_enable[6] = to_bool(value);
					break;
				case constfn_hash("logic_op_enable_7"):
					bs_desc.logic_op_enable[7] = to_bool(value);
					break;

				case constfn_hash("blend_op_0"):
					bs_desc.blend_op[0] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_1"):
					bs_desc.blend_op[1] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_2"):
					bs_desc.blend_op[2] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_3"):
					bs_desc.blend_op[3] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_4"):
					bs_desc.blend_op[4] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_5"):
					bs_desc.blend_op[5] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_6"):
					bs_desc.blend_op[6] = BlendDesc::to_op(value);
				case constfn_hash("blend_op_7"):
					bs_desc.blend_op[7] = BlendDesc::to_op(value);
					break;

				case constfn_hash("src_blend_0"):
					bs_desc.src_blend[0] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_1"):
					bs_desc.src_blend[1] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_2"):
					bs_desc.src_blend[2] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_3"):
					bs_desc.src_blend[3] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_4"):
					bs_desc.src_blend[4] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_5"):
					bs_desc.src_blend[5] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_6"):
					bs_desc.src_blend[6] = BlendDesc::to_factor(value);
				case constfn_hash("src_blend_7"):
					bs_desc.src_blend[7] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_0"):
					bs_desc.dst_blend[0] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_1"):
					bs_desc.dst_blend[1] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_2"):
					bs_desc.dst_blend[2] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_3"):
					bs_desc.dst_blend[3] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_4"):
					bs_desc.dst_blend[4] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_5"):
					bs_desc.dst_blend[5] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_6"):
					bs_desc.dst_blend[6] = BlendDesc::to_factor(value);
				case constfn_hash("dst_blend_7"):
					bs_desc.dst_blend[7] = BlendDesc::to_factor(value);
					break;

				case constfn_hash("blend_op_alpha_0"):
					bs_desc.blend_op_alpha[0] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_alpha_1"):
					bs_desc.blend_op_alpha[1] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_alpha_2"):
					bs_desc.blend_op_alpha[2] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_alpha_3"):
					bs_desc.blend_op_alpha[3] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_alpha_4"):
					bs_desc.blend_op_alpha[4] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_alpha_5"):
					bs_desc.blend_op_alpha[5] = BlendDesc::to_op(value);
					break;
				case constfn_hash("blend_op_alpha_6"):
					bs_desc.blend_op_alpha[6] = BlendDesc::to_op(value);
				case constfn_hash("blend_op_alpha_7"):
					bs_desc.blend_op_alpha[7] = BlendDesc::to_op(value);
					break;

				case constfn_hash("src_blend_alpha_0"):
					bs_desc.src_blend_alpha[0] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_alpha_1"):
					bs_desc.src_blend_alpha[1] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_alpha_2"):
					bs_desc.src_blend_alpha[2] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_alpha_3"):
					bs_desc.src_blend_alpha[3] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_alpha_4"):
					bs_desc.src_blend_alpha[4] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_alpha_5"):
					bs_desc.src_blend_alpha[5] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("src_blend_alpha_6"):
					bs_desc.src_blend_alpha[6] = BlendDesc::to_factor(value);
				case constfn_hash("src_blend_alpha_7"):
					bs_desc.src_blend_alpha[7] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_alpha_0"):
					bs_desc.dst_blend_alpha[0] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_alpha_1"):
					bs_desc.dst_blend_alpha[1] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_alpha_2"):
					bs_desc.dst_blend_alpha[2] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_alpha_3"):
					bs_desc.dst_blend_alpha[3] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_alpha_4"):
					bs_desc.dst_blend_alpha[4] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_alpha_5"):
					bs_desc.dst_blend_alpha[5] = BlendDesc::to_factor(value);
					break;
				case constfn_hash("dst_blend_alpha_6"):
					bs_desc.dst_blend_alpha[6] = BlendDesc::to_factor(value);
				case constfn_hash("dst_blend_alpha_7"):
					bs_desc.dst_blend_alpha[7] = BlendDesc::to_factor(value);
					break;

				case constfn_hash("color_write_mask_0"):
					bs_desc.color_write_mask[0] = to_uint8(value);
					break;
				case constfn_hash("color_write_mask_1"):
					bs_desc.color_write_mask[1] = to_uint8(value);
					break;
				case constfn_hash("color_write_mask_2"):
					bs_desc.color_write_mask[2] = to_uint8(value);
					break;
				case constfn_hash("color_write_mask_3"):
					bs_desc.color_write_mask[3] = to_uint8(value);
					break;
				case constfn_hash("color_write_mask_4"):
					bs_desc.color_write_mask[4] = to_uint8(value);
					break;
				case constfn_hash("color_write_mask_5"):
					bs_desc.color_write_mask[5] = to_uint8(value);
					break;
				case constfn_hash("color_write_mask_6"):
					bs_desc.color_write_mask[7] = to_uint8(value);
					break;
				case constfn_hash("color_write_mask_7"):
					bs_desc.color_write_mask[7] = to_uint8(value);
					break;
				}
			};
			for (auto & child_node : pass_node) {
				if (child_node.size() != 2)
					continue;
				try {
					auto first = white::Access<std::string>(*child_node.begin());
					auto second = white::Access<std::string>(*child_node.rbegin());
					auto first_hash = white::constfn_hash(first.c_str());

					switch (first_hash)
					{
					case constfn_hash("rs_mode"):
						rs_desc.mode = RasterizerDesc::to_mode<RasterizerMode>(second);
						break;
					case constfn_hash("cull"):
						rs_desc.cull = RasterizerDesc::to_mode<CullMode>(second);
						break;
					case constfn_hash("ccw"):
						rs_desc.ccw = to_bool(second);
						break;
					case constfn_hash("depth_clip_enable"):
						rs_desc.depth_clip_enable = to_bool(second);
						break;
					case constfn_hash("scissor_enable"):
						rs_desc.scissor_enable = to_bool(second);
						break;
					case constfn_hash("multisample_enable"):
						rs_desc.multisample_enable = to_bool(second);
						break;

					case constfn_hash("depth_enable"):
						ds_desc.depth_enable = to_bool(second);
						break;
					case constfn_hash("depth_write_mask"):
						ds_desc.depth_write_mask = to_bool(second);
						break;
					case constfn_hash("depth_func"):
						ds_desc.depth_func = DepthStencilDesc::to_op<CompareOp>(second);
						break;

					case constfn_hash("front_stencil_enable"):
						ds_desc.front_stencil_enable = to_bool(second);
						break;
					case constfn_hash("front_stencil_ref"):
						ds_desc.front_stencil_ref = to_uint16(second);
						break;
					case constfn_hash("front_stencil_read_mask"):
						ds_desc.front_stencil_read_mask = to_uint16(second);
						break;
					case constfn_hash("front_stencil_write_mask"):
						ds_desc.front_stencil_write_mask = to_uint16(second);
						break;
					case constfn_hash("front_stencil_fail"):
						ds_desc.front_stencil_fail = DepthStencilDesc::to_op<StencilOp>(second);
						break;
					case constfn_hash("front_stencil_depth_fail"):
						ds_desc.front_stencil_depth_fail = DepthStencilDesc::to_op<StencilOp>(second);
						break;
					case constfn_hash("front_stencil_pass"):
						ds_desc.front_stencil_pass = DepthStencilDesc::to_op<StencilOp>(second);
						break;

					case constfn_hash("back_stencil_enable"):
						ds_desc.back_stencil_enable = to_bool(second);
						break;
					case constfn_hash("back_stencil_ref"):
						ds_desc.back_stencil_ref = to_uint16(second);
						break;
					case constfn_hash("back_stencil_read_mask"):
						ds_desc.back_stencil_read_mask = to_uint16(second);
						break;
					case constfn_hash("back_stencil_write_mask"):
						ds_desc.back_stencil_write_mask = to_uint16(second);
						break;
					case constfn_hash("back_stencil_fail"):
						ds_desc.back_stencil_fail = DepthStencilDesc::to_op<StencilOp>(second);
						break;
					case constfn_hash("back_stencil_depth_fail"):
						ds_desc.back_stencil_depth_fail = DepthStencilDesc::to_op<StencilOp>(second);
						break;
					case constfn_hash("back_stencil_pass"):
						ds_desc.back_stencil_pass = DepthStencilDesc::to_op<StencilOp>(second);
						break;

					case constfn_hash("blend_factor"):
						auto value = to_float4(second);
						bs_desc.blend_factor = LinearColor(value);
						break;
					case constfn_hash("sample_mask"):
						bs_desc.sample_mask = to_uint32(second);
						break;
					case constfn_hash("alpha_to_coverage_enable"):
						bs_desc.alpha_to_coverage_enable = to_bool(second);
						break;
					case constfn_hash("independent_blend_enable"):
						bs_desc.independent_blend_enable = to_bool(second);
						break;
					default:
						default_parse(first_hash, second);
					}
				}
				CatchIgnore(std::bad_any_cast &)
			}
		}

		void ComposePassShader(const asset::EffectTechniqueAsset&technique, asset::TechniquePassAsset& pass, scheme::TermNode& pass_node) {
			size_t seed = 0;
			auto macro_hash = white::combined_hash<ShaderMacro>();
			std::vector<ShaderMacro> macros{ GetAsset()->GetMacros() };
			for (auto & macro_pair : technique.GetMacros()) {
				white::hash_combine(seed, macro_hash(macro_pair));
				macros.emplace_back(macro_pair);
			}
			for (auto & macro_pair : pass.GetMacros()) {
				white::hash_combine(seed, macro_hash(macro_pair));
				macros.emplace_back(macro_pair);
			}

			for (auto & child_node : pass_node) {
				if (child_node.size() != 2)
					continue;
				try {
					auto first = white::Access<std::string>(*child_node.begin());
					auto second = white::Access<std::string>(*child_node.rbegin());
					auto first_hash = white::constfn_hash(first);

					asset::ShaderBlobAsset::Type compile_type;
					string_view compile_entry_point = second;
					size_t blob_hash = seed;
					white::hash_combine(blob_hash,white::constfn_hash(second));

					switch (first_hash) {
					case white::constfn_hash("vertex_shader"):
						compile_type = platform::Render::ShaderType::VertexShader;
						break;
					case white::constfn_hash("pixel_shader"):
						compile_type = platform::Render::ShaderType::PixelShader;
						break;
					default:
						continue;
					}
				
					using namespace white;

					auto path = Path().string();
#ifndef NDEBUG
					path += ".hlsl";
					{
						std::ofstream fout(path);
						fout << GetCode();
					}
#endif


					LOG_TRACE("ComposePassShader CompilerReflectStrip Type:{} ", first);
					ShaderCompilerInput input;
					input.Type = compile_type;
					input.EntryPoint = compile_entry_point;
					input.SourceName = path;

					asset::X::Shader::AppendCompilerEnvironment(input.Environment, input.Type);

					auto Code = asset::X::Shader::PreprocessShader(GetCode(), input);
					input.Code = Code.Code;

					auto pInfo = std::make_unique<ShaderInfo>(compile_type);

					auto blob = CompileAndReflect(input,
						D3DFlags::D3DCOMPILE_ENABLE_STRICTNESS |
#ifndef NDEBUG
						D3DFlags::D3DCOMPILE_DEBUG
#else
						D3DFlags::D3DCOMPILE_OPTIMIZATION_LEVEL3
#endif
						, pInfo.get()
					);
					blob = Strip(blob, compile_type, D3DFlags::D3DCOMPILER_STRIP_DEBUG_INFO
						| D3DFlags::D3DCOMPILER_STRIP_PRIVATE_DATA);

					GetAsset()->EmplaceBlob(blob_hash, asset::ShaderBlobAsset(compile_type, std::move(blob),pInfo.release()));
					pass.AssignOrInsertHash(compile_type, blob_hash);
				}
				CatchIgnore(std::bad_any_cast &)
			}
		}
	};

	std::shared_ptr<asset::EffectAsset> X::LoadEffectAsset(path const & effectpath)
	{
		return  AssetResourceScheduler::Instance().SyncLoad<EffectLoadingDesc>(effectpath);
	}
	std::shared_ptr<Render::Effect::Effect> platform::X::LoadEffect(std::string const & name)
	{
		return  AssetResourceScheduler::Instance().SyncSpawnResource<Render::Effect::Effect>(name);
	}
}




