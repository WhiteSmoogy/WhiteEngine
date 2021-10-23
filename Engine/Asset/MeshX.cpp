#include<WBase/typeinfo.h>
#include <WBase/wmathtype.hpp>
#include <WBase/id.hpp>

#include "../Core/AssetResourceScheduler.h"
#include "../Core/LFile.h"
#include "../Core/Coroutine/FileAsyncStream.h"
#include "../System/SystemEnvironment.h"

#include "MeshX.h"

namespace platform {
	//Mesh文件格式
	struct MeshHeader
	{
		//TODO Asset Common Header?
		white::uint32 Signature; //Magic Number == 'MESH'
		white::uint16 Machine;//资源所属的平台格式
		white::uint16 NumberOfSections;//主要的设计概念,SectionLoader将会加载出对应的资源,目前只实现一种Section
		union {
			white::uint16 SizeOfOptional;//必须保证对齐 %16 == 0
			white::uint16 FirstSectionOffset;//该偏移为Header的偏移,注意一定会进行对齐
		};
	};

	//每个Section的通用头
	struct SectionCommonHeader {
		white::uint16 SectionIndex;//这是当前的第几节
		white::uint32 NextSectionOffset;//下一节开始相对文件的偏移
		stdex::byte Name[8];//使用名字决定节加载器
		white::uint32 Size;//本节的大小,意味着节是由最大大小限制的,注意是包含填充大小的
	};

	wconstexpr white::uint32 GeomertyCurrentVersion = 0;

	struct GeomertySectionHeader :SectionCommonHeader {
		white::uint32 FourCC; //'LEME'
		white::uint32 Version;//保留支持
		white::uint32 CompressVersion;//保留支持,可能会引入内置压缩
		union {
			white::uint16 SizeOfOptional;//必须保证对齐
			white::uint16 GeomertyOffset;//该偏移为相对Header的偏移,注意一定会进行对齐
		};//保留支持，值应该是0
	};

	template<typename type>
	type Read(FileRead& file) {
		type value;
		file.Read(&value, sizeof(type));
		return value;
	}

	template<typename type>
	white::coroutine::Task<type> Read(white::coroutine::FileAsyncStream& stream)
	{
		type value;
		co_await stream.Read(&value, sizeof(type));
		co_return value;
	}


	class MeshSectionLoading {
	protected:
		using AssetType = asset::MeshAsset;
		std::shared_ptr<AssetType> mesh_asset;
		MeshSectionLoading(std::shared_ptr<AssetType> target)
			:mesh_asset(target) {
		}
	public:
		virtual std::experimental::generator<std::shared_ptr<AssetType>> Coroutine(FileRead& file) = 0;
		virtual white::coroutine::Task<void> GetAwaiter(white::coroutine::FileAsyncStream& file) = 0;
		virtual std::array<byte, 8> Name() = 0;
	};

	class GeomertySection :public MeshSectionLoading {
	public:
		GeomertySection(std::shared_ptr<AssetType> target)
			:MeshSectionLoading(target) {
		}
		std::array<byte, 8> Name() override {
			static std::array<byte, 8> name = { 'G','E','O','M','E','R','T','Y' };
			return name;
		}

		/*
		struct GeometrySection {
			white::uint8 VertexElmentsCount;
			Vertex::Element VertexElments[VertexElmentsCount];
			EFromat IndexFormat;//R16UI | R32UI
			white::uint16/32 VertexCount;
			white::uint16/32 IndexCount;

			byte[VertexElments[0...n].Size()][VertexCount];
			byte[IndexFormat.Size()][IndexCount];

			white::uint8 MeshesCount;
			struct SubMeshDescrption{
				white::uint8 MaterialIndex;
				white::uint8 LodsCount;
				AABB<float3> PosAABB;
				AABB<float2> TexAABB;
				strcut LodDescrption{
					white::uint16/32 VertexNum;
					white::uint16/32 VertexBase;
					white::uint16/32 IndexNum;
					white::uint16/32 IndexBase;
				}[LodsCount];
			}[MeshesCount]
		};
		*/
		std::experimental::generator<std::shared_ptr<AssetType>> Coroutine(FileRead& file) override {
			//read header
			GeomertySectionHeader header;//common header had read
			file.Read(&header.FourCC, sizeof(header.FourCC));
			if (header.FourCC != asset::four_cc_v < 'L', 'E', 'M', 'E'>)
				co_return;

			file.Read(&header.Version, sizeof(header.Version));
			if (header.Version != GeomertyCurrentVersion)
				co_return;

			file.Read(&header.CompressVersion, sizeof(header.CompressVersion));
			if (header.CompressVersion != 0)
				co_return;

			file.Read(&header.SizeOfOptional, sizeof(header.SizeOfOptional));
			if (header.SizeOfOptional != 0)
				co_return;

			file.Skip(header.SizeOfOptional);
			co_yield mesh_asset;

			white::uint8 VertexElmentsCount = Read<white::uint8>(file);
			auto & vertex_elements = mesh_asset->GetVertexElementsRef();
			vertex_elements.reserve(VertexElmentsCount);
			for (auto i = 0; i != VertexElmentsCount; ++i) {
				Render::Vertex::Element element;
				element.usage = (Render::Vertex::Usage)Read<byte>(file);
				element.usage_index = Read<byte>(file);
				element.format = Read<Render::EFormat>(file);
				vertex_elements.emplace_back(element);
			}
			auto index_format = Read<Render::EFormat>(file);
			mesh_asset->SetIndexFormat(index_format);

			auto index16bit = index_format == Render::EFormat::EF_R16UI;
			uint32 vertex_count = index16bit ? Read<uint16>(file) : Read<uint32>(file);
			uint32 index_count = index16bit ? Read<uint16>(file) : Read<uint32>(file);

			mesh_asset->SetVertexCount(vertex_count);
			mesh_asset->SetIndexCount(index_count);
			auto & vertex_streams = mesh_asset->GetVertexStreamsRef();
			for (auto i = 0; i != VertexElmentsCount; ++i) {
				auto vertex_stream = std::make_unique<stdex::byte[]>(vertex_elements[i].GetElementSize()*vertex_count);
				file.Read(vertex_stream.get(), vertex_elements[i].GetElementSize()*vertex_count);
				vertex_streams.emplace_back(std::move(vertex_stream));
			}

			auto index_stream = std::make_unique<stdex::byte[]>(Render::NumFormatBytes(index_format)*index_count);
			file.Read(index_stream.get(), Render::NumFormatBytes(index_format)*index_count);
			mesh_asset->GetIndexStreamsRef() = std::move(index_stream);

			auto & sub_meshes = mesh_asset->GetSubMeshDescesRef();
			auto sub_mesh_count = Read<white::uint8>(file);
			for (auto i = 0; i != sub_mesh_count; ++i) {
				asset::MeshAsset::SubMeshDescrption mesh_desc;
				mesh_desc.MaterialIndex = Read<white::uint8>(file);
				auto lods_count = Read<white::uint8>(file);

				auto pc = Read<white::math::data_storage<float, 3>>(file);
				auto po = Read<white::math::data_storage<float, 3>>(file);
				auto tc = Read<white::math::data_storage<float, 2>>(file);
				auto to = Read<white::math::data_storage<float, 2>>(file);

				for (auto lod_index = 0; lod_index != lods_count; ++lod_index) {
					asset::MeshAsset::SubMeshDescrption::LodDescription lod_desc;
					if (index16bit) {
						lod_desc.VertexNum = Read<white::uint16>(file);
						lod_desc.VertexBase = Read<white::uint16>(file);
						lod_desc.IndexNum = Read<white::uint16>(file);
						lod_desc.IndexBase = Read<white::uint16>(file);
					}
					else {
						lod_desc.VertexNum = Read<white::uint32>(file);
						lod_desc.VertexBase = Read<white::uint32>(file);
						lod_desc.IndexNum = Read<white::uint32>(file);
						lod_desc.IndexBase = Read<white::uint32>(file);
					}
					mesh_desc.LodsDescription.emplace_back(lod_desc);
				}
				sub_meshes.emplace_back(std::move(mesh_desc));
			}
		}

		white::coroutine::Task<void> GetAwaiter(white::coroutine::FileAsyncStream& file) override {
			//read header
			GeomertySectionHeader header;//common header had read
			co_await file.Read(&header.FourCC, sizeof(header.FourCC));
			if (header.FourCC != asset::four_cc_v < 'L', 'E', 'M', 'E'>)
				co_return;

			co_await file.Read(&header.Version, sizeof(header.Version));
			if (header.Version != GeomertyCurrentVersion)
				co_return;

			co_await file.Read(&header.CompressVersion, sizeof(header.CompressVersion));
			if (header.CompressVersion != 0)
				co_return;

			co_await file.Read(&header.SizeOfOptional, sizeof(header.SizeOfOptional));
			if (header.SizeOfOptional != 0)
				co_return;

			file.Skip(header.SizeOfOptional);

			white::uint8 VertexElmentsCount = co_await Read<white::uint8>(file);
			auto& vertex_elements = mesh_asset->GetVertexElementsRef();
			vertex_elements.reserve(VertexElmentsCount);
			for (auto i = 0; i != VertexElmentsCount; ++i) {
				Render::Vertex::Element element;
				element.usage = (Render::Vertex::Usage)co_await Read<byte>(file);
				element.usage_index = co_await Read<byte>(file);
				element.format = co_await Read<Render::EFormat>(file);
				vertex_elements.emplace_back(element);
			}
			auto index_format = co_await Read<Render::EFormat>(file);
			mesh_asset->SetIndexFormat(index_format);

			auto index16bit = index_format == Render::EFormat::EF_R16UI;
			uint32 vertex_count = index16bit ? co_await Read<uint16>(file) : co_await Read<uint32>(file);
			uint32 index_count = index16bit ? co_await Read<uint16>(file) : co_await Read<uint32>(file);

			mesh_asset->SetVertexCount(vertex_count);
			mesh_asset->SetIndexCount(index_count);
			auto& vertex_streams = mesh_asset->GetVertexStreamsRef();
			for (auto i = 0; i != VertexElmentsCount; ++i) {
				auto vertex_stream = std::make_unique<stdex::byte[]>(vertex_elements[i].GetElementSize() * vertex_count);
				co_await file.Read(vertex_stream.get(), vertex_elements[i].GetElementSize() * vertex_count);
				vertex_streams.emplace_back(std::move(vertex_stream));
			}

			auto index_stream = std::make_unique<stdex::byte[]>(Render::NumFormatBytes(index_format) * index_count);
			co_await file.Read(index_stream.get(), Render::NumFormatBytes(index_format) * index_count);
			mesh_asset->GetIndexStreamsRef() = std::move(index_stream);

			auto& sub_meshes = mesh_asset->GetSubMeshDescesRef();
			auto sub_mesh_count = co_await Read<white::uint8>(file);
			for (auto i = 0; i != sub_mesh_count; ++i) {
				asset::MeshAsset::SubMeshDescrption mesh_desc;
				mesh_desc.MaterialIndex = co_await Read<white::uint8>(file);
				auto lods_count = co_await Read<white::uint8>(file);

				auto pc =co_await Read<white::math::data_storage<float, 3>>(file);
				auto po =co_await Read<white::math::data_storage<float, 3>>(file);
				auto tc =co_await Read<white::math::data_storage<float, 2>>(file);
				auto to =co_await Read<white::math::data_storage<float, 2>>(file);

				for (auto lod_index = 0; lod_index != lods_count; ++lod_index) {
					asset::MeshAsset::SubMeshDescrption::LodDescription lod_desc;
					if (index16bit) {
						lod_desc.VertexNum = co_await Read<white::uint16>(file);
						lod_desc.VertexBase = co_await Read<white::uint16>(file);
						lod_desc.IndexNum = co_await Read<white::uint16>(file);
						lod_desc.IndexBase = co_await Read<white::uint16>(file);
					}
					else {
						lod_desc.VertexNum = co_await Read<white::uint32>(file);
						lod_desc.VertexBase = co_await Read<white::uint32>(file);
						lod_desc.IndexNum = co_await Read<white::uint32>(file);
						lod_desc.IndexBase = co_await Read<white::uint32>(file);
					}
					mesh_desc.LodsDescription.emplace_back(lod_desc);
				}
				sub_meshes.emplace_back(std::move(mesh_desc));
			}
		}
	};

	template<typename... section_types>
	class MeshLoadingDesc : public asset::AssetLoading<asset::MeshAsset> {
	private:
		struct MeshDesc {
			X::path mesh_path;
			std::shared_ptr<AssetType> mesh_asset;
			std::vector<std::unique_ptr<MeshSectionLoading>> section_loaders;
		} mesh_desc;
	public:
		explicit MeshLoadingDesc(X::path const & meshpath)
		{
			mesh_desc.mesh_path = meshpath;
			mesh_desc.mesh_asset = std::make_shared<AssetType>();
			ForEachSectionTypeImpl<std::tuple<section_types...>>(white::make_index_sequence<sizeof...(section_types)>());
		}

		template<typename tuple, size_t... indices>
		void ForEachSectionTypeImpl(white::index_sequence<indices...>) {
			int ignore[] = { (static_cast<void>(
				mesh_desc.section_loaders
				.emplace_back(
					std::make_unique<std::tuple_element_t<indices,tuple>>(mesh_desc.mesh_asset))),0)
				...
			};
			(void)ignore;
		}

		std::size_t Type() const override {
			return white::type_id<MeshLoadingDesc>().hash_code();
		}

		std::size_t Hash() const override {
			return white::hash_combine_seq(Type(), mesh_desc.mesh_path.wstring());
		}

		const asset::path& Path() const override {
			return mesh_desc.mesh_path;
		}

		std::experimental::generator<std::shared_ptr<AssetType>> Coroutine() override {
			platform_ex::Windows::File interna_file{ mesh_desc.mesh_path.wstring(),platform::File::kToRead };

			FileRead file{ interna_file };

			MeshHeader header;
			header.Signature = Read<white::uint32>(file);
			if (header.Signature != asset::four_cc_v <'M', 'E', 'S', 'H'>)
				co_return;

			header.Machine = Read<white::uint16>(file);
			header.NumberOfSections = Read<white::uint16>(file);
			header.FirstSectionOffset = Read<white::uint16>(file);

			file.Skip(header.FirstSectionOffset);

			for (auto i = 0; i != header.NumberOfSections; ++i) {
				SectionCommonHeader common_header;
				common_header.SectionIndex = Read<white::uint16>(file);
				common_header.NextSectionOffset = Read<white::uint32>(file);
				file.Read(common_header.Name, sizeof(common_header.Name));
				common_header.Size = Read<white::uint32>(file);

				//find loader
				auto loader_iter = std::find_if(mesh_desc.section_loaders.begin(),
					mesh_desc.section_loaders.end(), [&](const std::unique_ptr<MeshSectionLoading>& pLoader) {
					auto name = pLoader->Name();
					for (auto j = 0; j != 8; ++j)
						if (name[j] != common_header.Name[j]) 
							return false;
					return true;
				});
				if (loader_iter != mesh_desc.section_loaders.end()) {
					for (auto iter : (*loader_iter)->Coroutine(file))
						co_yield nullptr;
				}

				file.SkipTo(common_header.NextSectionOffset);
			}
			co_yield mesh_desc.mesh_asset;
		}

		white::coroutine::Task<std::shared_ptr<AssetType>> GetAwaiter()
		{
			white::coroutine::FileAsyncStream file{ Environment->Scheduler->GetIOScheduler() ,mesh_desc.mesh_path,white::coroutine::file_share_mode::read };

			MeshHeader header;
			header.Signature =co_await Read<white::uint32>(file);
			if (header.Signature != asset::four_cc_v <'M', 'E', 'S', 'H'>)
				co_return nullptr;

			header.Machine = co_await Read<white::uint16>(file);
			header.NumberOfSections = co_await Read<white::uint16>(file);
			header.FirstSectionOffset = co_await Read<white::uint16>(file);

			file.Skip(header.FirstSectionOffset);

			for (auto i = 0; i != header.NumberOfSections; ++i) {
				SectionCommonHeader common_header;
				common_header.SectionIndex = co_await Read<white::uint16>(file);
				common_header.NextSectionOffset = co_await Read<white::uint32>(file);
				co_await file.Read(common_header.Name, sizeof(common_header.Name));
				common_header.Size = co_await Read<white::uint32>(file);

				//find loader
				auto loader_iter = std::find_if(mesh_desc.section_loaders.begin(),
					mesh_desc.section_loaders.end(), [&](const std::unique_ptr<MeshSectionLoading>& pLoader) {
						auto name = pLoader->Name();
						for (auto j = 0; j != 8; ++j)
							if (name[j] != common_header.Name[j])
								return false;
						return true;
					});
				if (loader_iter != mesh_desc.section_loaders.end()) {
					co_await(*loader_iter)->GetAwaiter(file);
				}

				file.SkipTo(common_header.NextSectionOffset);
			}

			co_return mesh_desc.mesh_asset;
		}
	};


	std::shared_ptr<asset::MeshAsset> X::LoadMeshAsset(path const& meshpath) {
		return  platform::AssetResourceScheduler::Instance().SyncLoad<MeshLoadingDesc<GeomertySection>>(meshpath);
	}
	std::shared_ptr<Mesh> X::LoadMesh(path const & meshpath, const std::string & name)
	{
		return  platform::AssetResourceScheduler::Instance().SyncSpawnResource<Mesh>(meshpath, name);
	}

	white::coroutine::Task<std::shared_ptr<asset::MeshAsset>> platform::X::AsyncLoadMeshAsset(path const& meshpath)
	{
		return platform::AssetResourceScheduler::Instance().AsyncLoad<MeshLoadingDesc<GeomertySection>>(meshpath);
	}
	
}