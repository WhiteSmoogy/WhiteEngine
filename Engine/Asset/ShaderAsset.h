#pragma once

#include <WBase/wmacro.h>
#include <WBase/winttype.hpp>
#include <WBase/any.h>

#include <string>
#include <vector>
#include <utility>
#include <variant>
#include <filesystem>

#include "../Render/PipleState.h"
#include "../Render/Effect/Effect.hpp"
#include "../Core/Coroutine/Task.h"

namespace asset
{
	using platform::Render::ShaderParamType;
	using platform::Render::ShaderMacro;

	class AssetName {
	public:
		DefGetter(const wnothrow, const std::string&, Name, name)
			DefGetter(const wnothrow, std::size_t, NameHash, hash)
			void SetName(const std::string&);
	private:
		std::string name;
		std::size_t hash;
	};

	class BindDesc
	{
	public:
		enum {Any = -1};

		int GetIndex() const
		{
			return register_index;
		}

		int GetSpace() const
		{
			return register_space;
		}

		void SetIndex(int index)
		{
			register_index =index;
		}

		void SetSpace(int space)
		{
			register_space = space;
		}
	protected:
		int register_index =Any;
		int register_space =Any;
	};

	class ShaderConstantBufferAsset : public AssetName,public BindDesc {
	public:
		DefGetter(const wnothrow, const std::vector<white::uint32>&, ParamIndices, indices)
			DefGetter(wnothrow, std::vector<white::uint32>&, ParamIndicesRef, indices)

		DefGetter(const wnothrow, const std::string&, ElemInfo, elem_info)
		DefGetter(wnothrow, std::string&, ElemInfoRef, elem_info)
	private:
		std::vector<white::uint32> indices;

		std::string elem_info;
	};

	class ShaderParameterAsset : public AssetName, public BindDesc {
	public:
		DefGetter(const wnothrow, const ShaderParamType&, Type, type)
			DefGetter(wnothrow, ShaderParamType&, TypeRef, type)
			DefGetter(const wnothrow, white::uint32, ArraySize, array_size)
			DefGetter(wnothrow, white::uint32&, ArraySizeRef, array_size)

			DefGetter(const wnothrow, const ShaderParamType&, ElemType, std::get<ShaderParamType>(elem_info))
			DefGetter(wnothrow, ShaderParamType&, ElemTypeRef, std::get<ShaderParamType>(elem_info))

			DefGetter(const wnothrow, const std::string&, ElemUserType, std::get<std::string>(elem_info))
			DefGetter(wnothrow, std::string&, ElemUserTypeRef, std::get<std::string>(elem_info))

			DefGetter(const wnothrow, const std::variant<ShaderParamType WPP_Comma std::string>&, ElemInfo, elem_info)
			DefGetter(wnothrow, std::variant<ShaderParamType WPP_Comma std::string>&, ElemInfoRef, elem_info)
	private:
		ShaderParamType type;
		white::uint32 array_size = 0;

		std::variant<ShaderParamType, std::string>  elem_info =platform::Render::SPT_ElemEmpty;

		
	};

	class ShaderFragmentAsset {
	public:
		DefGetter(const wnothrow, const std::string&, Fragment, fragment)
			DefGetter(wnothrow, std::string&, FragmentRef, fragment)

	private:
		std::string fragment;
	};

	class ShaderBlobAsset :white::noncopyable {
	public:
		using Type = platform::Render::ShaderType;
		using ShaderBlob = platform::Render::ShaderBlob;

		explicit ShaderBlobAsset(Type _type, ShaderBlob&& _blob, platform::Render::ShaderInfo* pinfo)
			:type(_type), pInfo(pinfo)
		{
			platform::Render::ShaderInitializer initializer;
			initializer.pBlob = &_blob;
			initializer.pInfo = pinfo;

			platform::Render::ShaderCompilerOutput Output;
			platform::Render::GenerateOuput(initializer, Output);

			code = Output.ShaderCode;
		}

		ShaderBlobAsset() = default;


		DefGetter(const wnothrow, const Type&, ShaderType, type)

			DefGetter(const wnothrow, const platform::Render::ShaderCode&, Code, code)


			DefGetter(wnothrow, platform::Render::ShaderCode&, Code, code)
			DefGetter(const wnothrow, const platform::Render::ShaderInfo&, Info, *pInfo)
			DefGetter(wnothrow, platform::Render::ShaderInfo&, Info, *pInfo)
	private:
		Type type;
		platform::Render::ShaderCode code;
		std::unique_ptr<platform::Render::ShaderInfo> pInfo;
	};



	class ShadersAsset
	{
	public:
		DefGetter(const wnothrow, const std::vector<ShaderMacro>&, Macros, macros)
			DefGetter(wnothrow, std::vector<ShaderMacro>&, MacrosRef, macros)

		DefGetter(const wnothrow, const std::vector<ShaderConstantBufferAsset>&, CBuffers, cbuffers)
			DefGetter(wnothrow, std::vector<ShaderConstantBufferAsset>&, CBuffersRef, cbuffers)

			DefGetter(const wnothrow, const std::vector<ShaderParameterAsset>&, Params, params)
			DefGetter(wnothrow, std::vector<ShaderParameterAsset>&, ParamsRef, params)

			DefGetter(const wnothrow, const std::vector<ShaderFragmentAsset>&, Fragments, fragements)
			DefGetter(wnothrow, std::vector<ShaderFragmentAsset>&, FragmentsRef, fragements)


		ShaderBlobAsset& GetBlob(size_t blob_index) {
			return blobs.find(blob_index)->second;
		}

		template<typename... Params>
		void EmplaceBlob(Params&&... params) {
			blobs.emplace(wforward(params)...);
		}

		template<typename T>
		void BindValue(size_t param_index, T&& value) {
			bind_values.emplace_back(param_index, wforward(value));
		}

		std::optional<std::any> GetValue(size_t param_index) {
			auto iter = std::find_if(bind_values.begin(), bind_values.end(), [&](const std::pair<size_t, std::any>& pair) {
				return pair.first == param_index;
				});
			if (iter != bind_values.end())
				return iter->second;
			return std::nullopt;
		}

		template<typename T>
		std::optional<T> GetInfo(const std::string_view& name) const;

		static std::string GetTypeName(ShaderParamType type);
		static ShaderParamType GetType(const std::string&);
		
	private:
		std::vector<ShaderMacro> macros;

		std::vector<ShaderConstantBufferAsset> cbuffers;
		std::vector<ShaderParameterAsset> params;
		std::vector<std::pair<size_t, std::any>> bind_values;
		std::vector<ShaderFragmentAsset> fragements;
		std::unordered_map<size_t, ShaderBlobAsset> blobs;
	public:
		std::string GenHLSLShader() const;


		enum GenSlot {
			MACRO,
			PARAM,
			CBUFFER,
			FRAGMENT
		};

		void EmplaceShaderGenInfo(GenSlot genslot, std::size_t localindex, std::size_t genindex) {
			gen_indices.emplace_back(genslot, static_cast<white::uint32>(localindex), static_cast<white::uint32>(genindex));
		}

		void PrepareShaderGen() {
			std::sort(gen_indices.begin(), gen_indices.end(), [](const std::tuple<GenSlot, white::uint32, white::uint32>& lhs, const std::tuple<GenSlot, white::uint32, white::uint32>& rhs) {
				if (std::get<2>(lhs) == std::get<2>(rhs))
					return std::get<1>(lhs) < std::get<1>(rhs);
				return std::get<2>(lhs) < std::get<2>(rhs);
				});
		}
	private:
		std::vector<std::tuple<GenSlot, white::uint32, white::uint32>> gen_indices;
	};

	bool RequireElemType(ShaderParamType type);

	bool RequireStructElemType(ShaderParamType type);
}

namespace platform::X {
	using path = std::filesystem::path;

	std::string GenHlslShader(const path& filepath);

	white::coroutine::Task<std::string> GenHlslShaderAsync(const path& filepath);
}

