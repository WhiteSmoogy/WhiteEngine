#pragma once

#include <WScheme/WScheme.h>
#include <WFramework/Core/ValueNode.h>
#include <WBase/type_traits.hpp>
#include "RenderInterface/RayTracingDefinitions.h"

#include <filesystem>
#include <fstream>

#include "ShaderAsset.h"
#include "WSLAssetX.h"
#include "Core/Coroutine/ReadOnlyFile.h"
#include "Core/Coroutine/IOScheduler.h"
#include "Core/Coroutine/SyncWait.h"
#include "../System/SystemEnvironment.h"

namespace platform::X
{
	using white::ValueNode;
	using path = std::filesystem::path;
	using namespace platform::Render::Shader;

	template<typename AssetType>
	class ShaderLoadingDesc{
	protected:
		struct ShaderDesc {
			path path;
			struct Data {
				scheme::TermNode term_node;
			};
			std::shared_ptr<Data> data;
			std::shared_ptr<AssetType> asset;
			std::string shader_code;
			bool is_ray_tracing;
		} shader_desc;

		static_assert(std::is_convertible_v<std::add_pointer_t<AssetType>, std::add_pointer_t<asset::ShadersAsset>>);
	protected:
		explicit ShaderLoadingDesc(X::path const& effectpath)
		{
			shader_desc.path = effectpath;
		}

		std::size_t Hash() const {
			return std::hash<std::wstring>()(shader_desc.path.wstring());
		}

		virtual const path& Path() const {
			return shader_desc.path;
		}

		virtual path Path(const path& input) const{
			return input;
		}

		std::shared_ptr<AssetType> ReturnValue()
		{
			return shader_desc.asset;
		}

		std::add_pointer_t<AssetType> GetAsset()
		{
			return shader_desc.asset.get();
		}

		scheme::TermNode& GetNode()
		{
			return shader_desc.data->term_node;
		}

		std::string& GetCode()
		{
			return shader_desc.shader_code;
		}

		void PreCreate()
		{
			shader_desc.data = std::make_shared<ShaderDesc::Data>();
			shader_desc.asset = std::make_shared<AssetType>();
			shader_desc.asset->SetName(shader_desc.path.string());
			shader_desc.is_ray_tracing = false;
		}

		void LoadNode()
		{
			shader_desc.data->term_node = *platform::X::LoadNode(Path(shader_desc.path)).begin();
		}

		white::coroutine::Task<void> LoadNodeAsync(white::coroutine::IOScheduler& io)
		{
			auto node = co_await LoadNodeAsync(io, shader_desc.path);

			if (node)
				shader_desc.data->term_node = *node.begin();
			else
				throw std::invalid_argument(shader_desc.path.string() + "is invalid");
		}

		void ParseNode()
		{
			white::coroutine::SyncWait(ParseNodeAsync(Environment->Scheduler->GetIOScheduler()));
		}

		white::coroutine::Task<> ParseNodeAsync(white::coroutine::IOScheduler& io)
		{
			auto& term_node = shader_desc.data->term_node;
			auto tag = white::Access<std::string>(*term_node.begin());
			shader_desc.is_ray_tracing = tag == "RayTracing";

			WAssert(tag == "effect" || tag == "RayTracing", R"(Invalid Format")");

			std::vector<std::pair<std::string, scheme::TermNode>> refers;
			co_await RecursiveReferNodeAsync(term_node, refers,io);

			auto new_node = white::MakeNode(white::MakeIndex(0));

			for (auto& pair : refers) {
				for (auto& node : pair.second) {
					new_node.try_emplace(white::MakeIndex(new_node), std::make_pair(node.begin(), node.end()), white::MakeIndex(new_node));
				}
			}

			for (auto& node : term_node)
				new_node.try_emplace(white::MakeIndex(new_node), std::make_pair(node.begin(), node.end()), white::MakeIndex(new_node));

			term_node = new_node;

			auto hash = [](auto param) {
				return std::hash<decltype(param)>()(param);
			};

			ParseMacro(shader_desc.asset->GetMacrosRef(), term_node, false);
			ParseConstatnBuffers(term_node);
			//parser params
			{
				auto param_nodes = term_node.SelectChildren([&](const scheme::TermNode& child) {
					if (child.empty())
						return false;
					try {
						return  AssetType::GetType(white::Access<std::string>(*child.begin())) != SPT_shader;
					}
					catch (white::unsupported&) {
						return false;
					}
					});
				for (auto& param_node : param_nodes)
					ParseParam(param_node, false);
			}
			//parser shader
			{
				auto fragments = X::SelectNodes("shader", term_node);
				for (auto& fragment : fragments) {
					shader_desc.asset->EmplaceShaderGenInfo(AssetType::FRAGMENT, shader_desc.asset->GetFragmentsRef().size(), std::stoul(fragment.GetName()));
					shader_desc.asset->GetFragmentsRef().emplace_back();
					shader_desc.asset->GetFragmentsRef().back().
						GetFragmentRef() = scheme::Deliteralize(
							white::Access<std::string>(*fragment.rbegin())
						);
				}

				auto includes = X::SelectNodes("include", term_node);
				for (auto& include : includes) {
					shader_desc.asset->EmplaceShaderGenInfo(AssetType::FRAGMENT, shader_desc.asset->GetFragmentsRef().size(), std::stoul(include.GetName()));
					shader_desc.asset->GetFragmentsRef().emplace_back();
					shader_desc.asset->GetFragmentsRef().back().
						GetFragmentRef() = GenIncludeShaderLine(
							white::Access<std::string>(*include.rbegin())
						);
				}
			}
			shader_desc.asset->PrepareShaderGen();
			shader_desc.shader_code = shader_desc.asset->GenHLSLShader();
		}

		std::string GenIncludeShaderLine(const std::string& path)
		{
			auto include_line = white::sfmt("#include \"%s\"", path.c_str());
			return include_line;
		}

		white::coroutine::Task<scheme::TermNode> LoadNodeAsync(white::coroutine::IOScheduler& io,const path& path) {
			auto file = white::coroutine::ReadOnlyFile::open(io, Path(path).string());

			constexpr size_t bufferSize = 4096;
			auto buffer = std::make_unique<char[]>(bufferSize);

			scheme::LexicalAnalyzer lexer;
			for (std::uint64_t offset = 0, fileSize = file.size(); offset < fileSize;)
			{
				const auto bytesToRead = static_cast<size_t>(
					std::min<std::uint64_t>(bufferSize, fileSize - offset));

				const auto bytesRead = co_await file.read(offset, buffer.get(), bytesToRead);

				co_await Environment->Scheduler->schedule();

				std::for_each(buffer.get(), buffer.get() + bytesRead, [&](char c) {lexer.ParseByte(c);});

				offset += bytesRead;
			}

			co_return scheme::SContext::Analyze(scheme::Tokenize(lexer.Literalize()));
		}

		void RecursiveReferNode(const ValueNode& effct_node, std::vector<std::pair<std::string, scheme::TermNode>>& includes) {
			auto refer_nodes = effct_node.SelectChildren([&](const scheme::TermNode& child) {
				if (child.size()) {
					return white::Access<std::string>(*child.begin()) == "refer";
				}
				return false;
				});
			for (auto& refer_node : refer_nodes) {
				auto path = white::Access<std::string>(*refer_node.rbegin());

				auto include_node = *platform::X::LoadNode(path).begin();
				RecursiveReferNode(include_node, includes);

				if (std::find_if(includes.begin(), includes.end(), [&path](const std::pair<std::string, scheme::TermNode>& pair)
					{
						return pair.first == path;
					}
				) == includes.end())
				{
					includes.emplace_back(path, include_node);
				}
			}
		}

		white::coroutine::Task<> RecursiveReferNodeAsync(const ValueNode& effct_node, std::vector<std::pair<std::string, scheme::TermNode>>& includes, white::coroutine::IOScheduler& io) {
			auto refer_nodes = effct_node.SelectChildren([&](const scheme::TermNode& child) {
				if (child.size()) {
					return white::Access<std::string>(*child.begin()) == "refer";
				}
				return false;
				});
			for (auto& refer_node : refer_nodes) {
				auto path = white::Access<std::string>(*refer_node.rbegin());

				auto include_node = *(co_await LoadNodeAsync(io,path)).begin();
				co_await RecursiveReferNodeAsync(include_node, includes,io);

				if (std::find_if(includes.begin(), includes.end(), [&path](const std::pair<std::string, scheme::TermNode>& pair)
					{
						return pair.first == path;
					}
				) == includes.end())
				{
					includes.emplace_back(path, include_node);
				}
			}
		}

		template<typename _type, typename _func>
		const _type& ReverseAccess(_func f, const scheme::TermNode& node) {
			auto it = std::find_if(node.rbegin(), node.rend(), f);
			return white::Access<_type>(*it);
		}

		template<typename _type>
		const _type& AccessLastNoChild(const scheme::TermNode& node) {
			return ReverseAccess<_type>([&](const scheme::TermNode& child) {return child.empty(); }, node);
		}

		std::string Access(const char* name, const scheme::TermNode& node) {
			auto it = std::find_if(node.begin(), node.end(), [&](const scheme::TermNode& child) {
				if (!child.empty())
					return white::Access<std::string>(*child.begin()) == name;
				return false;
				});
			return white::Access<std::string>(*(it->rbegin()));
		}

		white::observer_ptr<const string> AccessPtr(const char* name, const scheme::TermNode& node) {
			auto it = std::find_if(node.begin(), node.end(), [&](const scheme::TermNode& child) {
				if (!child.empty())
					return white::Access<std::string>(*child.begin()) == name;
				return false;
				});
			if (it != node.end())
				return white::AccessPtr<std::string>(*(it->rbegin()));
			else
				return nullptr;
		};

		void ParseConstatnBuffers(const scheme::TermNode& term_node)
		{
			auto cbuffer_nodes = SelectNodes("cbuffer", term_node);
			for (auto& cbuffer_node : cbuffer_nodes) {
				asset::ShaderConstantBufferAsset cbuffer;
				cbuffer.SetName(AccessLastNoChild<std::string>(cbuffer_node));

				auto index_space = ParseBind(cbuffer_node);
				cbuffer.SetIndex(index_space.first);
				cbuffer.SetSpace(index_space.second);

				auto param_nodes = cbuffer_node.SelectChildren([&](const scheme::TermNode& child) {
					if (child.empty())
						return false;
					try {
						return  AssetType::GetType(white::Access<std::string>(*child.begin())) != SPT_shader;
					}
					catch (white::unsupported&) {
						return false;
					}
					});
				std::vector<white::uint32> ParamIndices;
				for (auto& param_node : param_nodes) {
					auto param_index = ParseParam(param_node, true);
					ParamIndices.emplace_back(static_cast<white::uint32>(param_index));
				}
				cbuffer.GetParamIndicesRef() = std::move(ParamIndices);

				shader_desc.asset->EmplaceShaderGenInfo(AssetType::CBUFFER, shader_desc.asset->GetCBuffersRef().size(), std::stoul(cbuffer_node.GetName()));
				shader_desc.asset->GetCBuffersRef().emplace_back(std::move(cbuffer));
			}
			auto cbuffer_template_nodes = SelectNodes("ConstantBuffer", term_node);
			for (auto& cbuffer_node : cbuffer_template_nodes) {
				asset::ShaderConstantBufferAsset cbuffer;
				cbuffer.SetName(AccessLastNoChild<std::string>(cbuffer_node));

				auto index_space = ParseBind(cbuffer_node);
				cbuffer.SetIndex(index_space.first);
				cbuffer.SetSpace(index_space.second);

				if (auto elemtype = AccessPtr("elemtype", cbuffer_node)) {
					cbuffer.GetElemInfoRef() = *elemtype;
				}

				shader_desc.asset->EmplaceShaderGenInfo(AssetType::CBUFFER, shader_desc.asset->GetCBuffersRef().size(), std::stoul(cbuffer_node.GetName()));
				shader_desc.asset->GetCBuffersRef().emplace_back(std::move(cbuffer));
			}
		}

		void ParseMacro(std::vector<asset::ShaderMacro>& macros, const scheme::TermNode& node, bool topmacro)
		{
			//macro (macro (name foo) (value bar))
			auto macro_nodes = X::SelectNodes("macro", node);
			for (auto& macro_node : macro_nodes) {
				if (!topmacro) {
					shader_desc.asset->EmplaceShaderGenInfo(AssetType::MACRO, macros.size(), std::stoul(macro_node.GetName()));
				}
				else {
					shader_desc.asset->EmplaceShaderGenInfo(AssetType::MACRO, macros.size(), 0);
				}
				macros.emplace_back(
					Access("name", macro_node),
					Access("value", macro_node));
			}
			UniqueMacro(macros);
		}

		void UniqueMacro(std::vector<asset::ShaderMacro>& macros)
		{
			//TODO! remap shader gen info
			auto iter = macros.begin();
			while (iter != macros.end()) {
				auto exist = std::find_if(iter + 1, macros.end(), [&iter](const asset::ShaderMacro& tail)
					{
						return iter->first == tail.first;
					}
				);
				if (exist != macros.end())
					iter = macros.erase(iter);
				else
					++iter;
			}
		}

		size_t ParseParam(const scheme::TermNode& param_node, bool cbuffer_param) {
			asset::ShaderParameterAsset param;
			param.SetName(AccessLastNoChild<std::string>(param_node));
			//don't need check type again
			param.GetTypeRef() = AssetType::GetType(white::Access<std::string>(*param_node.begin()));

			if (!cbuffer_param) {
				auto index_space = ParseBind(param_node);
				param.SetIndex(index_space.first);
				param.SetSpace(index_space.second);
			}

			if (IsNumberType(param.GetType())) {
				if (auto p = white::AccessChildPtr<std::string>(param_node, "arraysize"))
					param.GetArraySizeRef() = std::stoul(*p);
			}
			else if (IsElemType(param.GetType())) {
				if (auto elemtype = AccessPtr("elemtype", param_node)) {
					try {
						param.GetElemInfoRef() = AssetType::GetType(*elemtype);
					}
					catch (white::unsupported&) {
						param.GetElemInfoRef() = *elemtype;
					}
				}
			}
			auto index = shader_desc.asset->GetParams().size();
			auto optional_value = ReadParamValue(param_node, param.GetType());
			if (optional_value.has_value())
				shader_desc.asset->BindValue(index, optional_value.value());
			if (!cbuffer_param)
				shader_desc.asset->EmplaceShaderGenInfo(AssetType::PARAM, index, std::stoul(param_node.GetName()));
			shader_desc.asset->GetParamsRef().emplace_back(std::move(param));
			return index;
		}

		std::pair<int,int> ParseBind(const scheme::TermNode& param_node) {
			std::pair<int, int> index_space{asset::BindDesc::Any,asset::BindDesc::Any };

			//(register {index} (space {sapce}))
			if (auto pRegNode = X::SelectNode("register", param_node))
			{
				auto index = white::Access<std::string>(*std::next(pRegNode->begin()));

				index_space.first = to_uint8(index);

				if (auto pSpaceNode = X::SelectNode("space", param_node))
				{
					auto space = white::Access<std::string>(*std::next(pSpaceNode->begin()));

					index_space.second = MapSpace(space);
				}
			}
			else if (auto pSpaceNode = X::SelectNode("space", param_node))
			{
				auto space = white::Access<std::string>(*std::next(pSpaceNode->begin()));

				index_space.second = MapSpace(space);
			}

			return index_space;
		}

		static int MapSpace(const std::string& value)
		{
			auto hash = white::constfn_hash(value);

			switch (hash)
			{
			case white::constfn_hash("RAY_TRACING_REGISTER_SPACE_LOCAL"):
				return RAY_TRACING_REGISTER_SPACE_LOCAL;
			case white::constfn_hash("RAY_TRACING_REGISTER_SPACE_GLOBAL"):
				return RAY_TRACING_REGISTER_SPACE_GLOBAL;
			case white::constfn_hash("RAY_TRACING_REGISTER_SPACE_SYSTEM"):
				return RAY_TRACING_REGISTER_SPACE_SYSTEM;
			}

			return -1;
		}

		static white::math::float4 to_float4(const std::string& value) {
			white::math::float4 result;
			auto iter = result.begin();
			white::split(value.begin(), value.end(),
				[](char c) {return c == ','; },
				[&](decltype(value.begin()) b, decltype(value.end()) e) {
					auto v = std::string(b, e);
					*iter = std::stof(v);
					++iter;
				}
			);
			return result;
		};

		static white::uint8 to_uint8(const std::string& value) {
			return static_cast<white::uint8>(std::stoul(value));
		};

		std::optional<std::any> ReadParamValue(const scheme::TermNode& param_node, asset::ShaderParamType type) {
#define AccessParam(name,expr) if (auto value = AccessPtr(name, param_node)) expr
			using namespace Render;
			if (type == SPT_sampler) {
				TextureSampleDesc sampler;
				AccessParam("border_clr", sampler.border_clr = LinearColor(to_float4(*value)));

				AccessParam("address_mode_u", sampler.address_mode_u = TextureSampleDesc::to_mode(*value));
				AccessParam("address_mode_v", sampler.address_mode_v = TextureSampleDesc::to_mode(*value));
				AccessParam("address_mode_w", sampler.address_mode_w = TextureSampleDesc::to_mode(*value));

				AccessParam("filtering", sampler.filtering = TextureSampleDesc::to_op<TexFilterOp>(*value));

				AccessParam("max_anisotropy", sampler.max_anisotropy = to_uint8(*value));

				AccessParam("min_lod", sampler.min_lod = std::stof(*value));
				AccessParam("max_lod", sampler.max_lod = std::stof(*value));
				AccessParam("mip_map_lod_bias", sampler.mip_map_lod_bias = std::stof(*value));

				AccessParam("cmp_func", sampler.cmp_func = TextureSampleDesc::to_op<CompareOp>(*value));

				return std::any(sampler);
			}
#undef AccessParam
			return std::nullopt;
		}
	};
}