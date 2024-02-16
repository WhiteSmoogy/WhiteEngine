#include<WBase/typeinfo.h>
#include <WBase/wmathtype.hpp>
#include <WBase/id.hpp>

#include "Core/Coroutine/AsyncStream.h"

#include "Runtime/AssetResourceScheduler.h"
#include "Runtime/LFile.h"
#include "Runtime/MemStack.h"

#include "System/SystemEnvironment.h"
#include "RenderInterface/IContext.h"

#include "MeshX.h"

#include <ranges>

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
		byte Name[8];//使用名字决定节加载器
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

	template<typename type,typename AsyncStream>
	white::coroutine::Task<type> Read(AsyncStream& stream)
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
		virtual white::coroutine::Task<void> GetAwaiter(white::coroutine::FileAsyncStream& file) = 0;
		virtual white::coroutine::Task<void> GetAwaiter(white::coroutine::MemoryAsyncReadStream& memory) = 0;
		virtual std::array<byte, 8> Name() = 0;

		void ReBind(std::shared_ptr<AssetType> target)
		{
			mesh_asset = target;
		}
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
		template<typename AsyncStream>
		white::coroutine::Task<void> GetAwaiter(AsyncStream& stream) {
			//read header
			GeomertySectionHeader header;//common header had read
			co_await stream.Read(&header.FourCC, sizeof(header.FourCC));
			if (header.FourCC != asset::four_cc_v < 'L', 'E', 'M', 'E'>)
				co_return;

			co_await stream.Read(&header.Version, sizeof(header.Version));
			co_await stream.Read(&header.CompressVersion, sizeof(header.CompressVersion));
			co_await stream.Read(&header.SizeOfOptional, sizeof(header.SizeOfOptional));

			stream.Skip(header.SizeOfOptional);

			white::uint8 VertexElmentsCount = co_await Read<white::uint8>(stream);
			auto& vertex_elements = mesh_asset->GetVertexElementsRef();
			vertex_elements.reserve(VertexElmentsCount);
			for (auto i = 0; i != VertexElmentsCount; ++i) {
				Render::Vertex::Element element;
				element.usage = (Render::Vertex::Usage)co_await Read<byte>(stream);
				element.usage_index = co_await Read<byte>(stream);
				element.format = co_await Read<Render::EFormat>(stream);
				vertex_elements.emplace_back(element);
			}
			auto index_format = co_await Read<Render::EFormat>(stream);
			mesh_asset->SetIndexFormat(index_format);

			auto index16bit = index_format == Render::EFormat::EF_R16UI;
			uint32 vertex_count = index16bit ? co_await Read<uint16>(stream) : co_await Read<uint32>(stream);
			uint32 index_count = index16bit ? co_await Read<uint16>(stream) : co_await Read<uint32>(stream);

			mesh_asset->SetVertexCount(vertex_count);
			mesh_asset->SetIndexCount(index_count);
			auto& vertex_streams = mesh_asset->GetVertexStreamsRef();
			for (auto i = 0; i != VertexElmentsCount; ++i) {
				auto vertex_stream = std::make_unique<byte[]>(vertex_elements[i].GetElementSize() * vertex_count);
				co_await stream.Read(vertex_stream.get(), vertex_elements[i].GetElementSize() * vertex_count);
				vertex_streams.emplace_back(std::move(vertex_stream));
			}

			auto index_stream = std::make_unique<byte[]>(Render::NumFormatBytes(index_format) * index_count);
			co_await stream.Read(index_stream.get(), Render::NumFormatBytes(index_format) * index_count);
			mesh_asset->GetIndexStreamsRef() = std::move(index_stream);

			auto& sub_meshes = mesh_asset->GetSubMeshDescesRef();
			auto sub_mesh_count = co_await Read<white::uint8>(stream);
			for (auto i = 0; i != sub_mesh_count; ++i) {
				asset::MeshAsset::SubMeshDescrption mesh_desc;
				mesh_desc.MaterialIndex = co_await Read<white::uint8>(stream);
				auto lods_count = co_await Read<white::uint8>(stream);

				/*pc po tc to*/
				co_await Read<white::math::data_storage<float, 10>>(stream);

				for (auto lod_index = 0; lod_index != lods_count; ++lod_index) {
					asset::MeshAsset::SubMeshDescrption::LodDescription lod_desc;
					if (index16bit) {
						lod_desc.VertexNum = co_await Read<white::uint16>(stream);
						lod_desc.VertexBase = co_await Read<white::uint16>(stream);
						lod_desc.IndexNum = co_await Read<white::uint16>(stream);
						lod_desc.IndexBase = co_await Read<white::uint16>(stream);
					}
					else {
						lod_desc.VertexNum = co_await Read<white::uint32>(stream);
						lod_desc.VertexBase = co_await Read<white::uint32>(stream);
						lod_desc.IndexNum = co_await Read<white::uint32>(stream);
						lod_desc.IndexBase = co_await Read<white::uint32>(stream);
					}
					mesh_desc.LodsDescription.emplace_back(lod_desc);
				}
				sub_meshes.emplace_back(std::move(mesh_desc));
			}
		}

		white::coroutine::Task<void> GetAwaiter(white::coroutine::FileAsyncStream& file) override
		{
			return GetAwaiter<white::coroutine::FileAsyncStream>(file);
		}

		white::coroutine::Task<void> GetAwaiter(white::coroutine::MemoryAsyncReadStream& file) override
		{
			return GetAwaiter<white::coroutine::MemoryAsyncReadStream>(file);
		}
	};

	using SectionLoaders = std::vector<std::unique_ptr<MeshSectionLoading>>;

	template<typename... section_types>
	class MeshLoadingDesc : public asset::AssetLoading<asset::MeshAsset> {
	private:
		struct MeshDesc {
			X::path mesh_path;
			SectionLoaders section_loaders;
		} mesh_desc;
	public:
		explicit MeshLoadingDesc(X::path const & meshpath)
		{
			mesh_desc.mesh_path = meshpath;
			ForEachSectionTypeImpl<std::tuple<section_types...>>(white::make_index_sequence<sizeof...(section_types)>());
		}

		template<typename tuple, size_t... indices>
		void ForEachSectionTypeImpl(white::index_sequence<indices...>) {
			int ignore[] = { (static_cast<void>(
				mesh_desc.section_loaders
				.emplace_back(
					std::make_unique<std::tuple_element_t<indices,tuple>>(std::shared_ptr<asset::MeshAsset>{}))),0)
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

		template<typename AsyncStream>
		static white::coroutine::Task<std::shared_ptr<asset::MeshAsset>> GetAwaiter(AsyncStream& stream, SectionLoaders& section_loaders)
		{
			MeshHeader header;
			sizeof(header);
			header.Signature = co_await Read<white::uint32>(stream);
			if (header.Signature != asset::four_cc_v <'M', 'E', 'S', 'H'>)
				co_return nullptr;
			header.Machine = co_await Read<white::uint16>(stream);
			header.NumberOfSections = co_await Read<white::uint16>(stream);
			header.FirstSectionOffset = co_await Read<white::uint16>(stream);

			auto mesh_asset = std::make_shared<asset::MeshAsset>();

			stream.Skip(header.FirstSectionOffset);

			for (auto i = 0; i != header.NumberOfSections; ++i) {
				SectionCommonHeader common_header;
				common_header.SectionIndex = co_await Read<white::uint16>(stream);
				common_header.NextSectionOffset = co_await Read<white::uint32>(stream);
				co_await stream.Read(common_header.Name, sizeof(common_header.Name));
				common_header.Size = co_await Read<white::uint32>(stream);

				//find loader
				auto loader_iter = std::find_if(section_loaders.begin(),
					section_loaders.end(), [&](const std::unique_ptr<MeshSectionLoading>& pLoader) {
						auto name = pLoader->Name();
						for (auto j = 0; j != 8; ++j)
							if (name[j] != common_header.Name[j])
								return false;
						return true;
					});
				if (loader_iter != section_loaders.end()) {
					(*loader_iter)->ReBind(mesh_asset);
					co_await(*loader_iter)->GetAwaiter(stream);
				}

				stream.SkipTo(common_header.NextSectionOffset);
			}

			co_return mesh_asset;
		}

		white::coroutine::Task<std::shared_ptr<AssetType>> GetAwaiter() override
		{
			white::coroutine::FileAsyncStream file{ Environment->Scheduler->GetIOScheduler() ,mesh_desc.mesh_path,white::coroutine::file_share_mode::read };

			auto asset = co_await GetAwaiter<white::coroutine::FileAsyncStream>(file, mesh_desc.section_loaders);

			co_return asset;
		}
	};

	template<typename... section_types>
	class BatchMeshLoadingDesc
	{
	private:
		struct MeshDesc {
			white::span<const X::path> pathes;
			white::span<std::shared_ptr<asset::MeshAsset>> assets;
			std::vector<std::unique_ptr<MeshSectionLoading>> section_loaders;
			WhiteEngine::MemStackBase memory;
		} mesh_desc;
	public:
		explicit BatchMeshLoadingDesc(white::span<const X::path> pathes, white::span<std::shared_ptr<asset::MeshAsset>> asset)
		{
			mesh_desc.pathes = pathes;
			mesh_desc.assets = asset;
			ForEachSectionTypeImpl<std::tuple<section_types...>>(white::make_index_sequence<sizeof...(section_types)>(), {});
		}

		template<typename tuple, size_t... indices>
		void ForEachSectionTypeImpl(white::index_sequence<indices...>, std::shared_ptr<asset::MeshAsset> asset) {
			int ignore[] = { (static_cast<void>(
				mesh_desc.section_loaders
				.emplace_back(
					std::make_unique<std::tuple_element_t<indices,tuple>>(asset))),0)
				...
			};
			(void)ignore;
		}

		white::coroutine::Task<void> GetAwaiter()
		{
			auto& dstorage = platform::Render::Context::Instance().GetDevice().GetDStorage();

			std::vector<std::shared_ptr<platform_ex::DStorageFile> > files;
			for (auto& path : mesh_desc.pathes)
			{
				files.emplace_back(dstorage.OpenFile(path));
			}

			std::vector<white::observer_ptr<byte>> buffers;
			for (auto& file : files)
			{
				buffers.emplace_back(white::make_observer(new (mesh_desc.memory) byte[file->file_size]));
			}

			for (auto index : std::views::iota(0u, buffers.size()))
			{
				platform_ex::DStorageFile2MemoryRequest Request;
				Request.File = {
					.Source = files[index],
					.Offset = 0,
					.Size = static_cast<uint32>(files[index]->file_size),
				};
				Request.Memory = {
					.Buffer = buffers[index].get(),
					.Size = Request.File.Size
				};

				dstorage.EnqueueRequest(Request);
			}

			co_await dstorage.SubmitUpload(platform_ex::DStorageQueueType::Memory);

			for (auto index : std::views::iota(0u, buffers.size()))
			{
				white::coroutine::MemoryAsyncReadStream stream{ white::make_span(buffers[index].get(),files[index]->file_size) };

				mesh_desc.assets[index] = co_await MeshLoadingDesc<section_types...>::GetAwaiter(stream, mesh_desc.section_loaders);
			}
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

	white::coroutine::Task<void> platform::X::BatchLoadMeshAsset(white::span<const X::path> pathes, white::span<std::shared_ptr<asset::MeshAsset>> asset)
	{
		BatchMeshLoadingDesc<GeomertySection> desc{ pathes,asset };

		co_await desc.GetAwaiter();;
	}

	
}