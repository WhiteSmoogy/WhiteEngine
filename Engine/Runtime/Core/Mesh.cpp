
#include <WBase/smart_ptr.hpp>
#include "Mesh.h"
#include "RenderInterface/IContext.h"
#include "AssetResourceScheduler.h"
#include "../Asset/MeshX.h"
#include "RenderInterface/IRayDevice.h"
#include "RenderInterface/IRayContext.h"
#include "../System/SystemEnvironment.h"

namespace platform {
	using namespace Render;

	Mesh::Mesh(const asset::MeshAsset & asset, const std::string & _name)
		:name(_name), sub_meshes(asset.GetSubMeshDesces())
	{
		auto& device = Context::Instance().GetDevice();
		input_layout = unique_raw(device.CreateInputLayout());

		auto index_stream = white::share_raw(
			device.CreateIndexBuffer(
				Buffer::Usage::Static, EAccessHint::EA_GPURead | EAccessHint::EA_Immutable,
				NumFormatBytes(asset.GetIndexFormat()) * asset.GetIndexCount(),
				asset.GetIndexFormat(), asset.GetIndexStreams().get()));

		min = white::math::float3();
		max = white::math::float3();
		for (std::size_t i = 0; i != asset.GetVertexElements().size(); ++i) {
			auto& element = asset.GetVertexElements()[i];
			auto& stream = asset.GetVertexStreams()[i];
			auto vertex_stream = white::share_raw(
				device.CreateVertexBuffer(
					Buffer::Usage::Static,
					EAccessHint::EA_GPURead | EAccessHint::EA_Immutable,
					element.GetElementSize()*asset.GetVertexCount(),
					element.format, stream.get()));
			input_layout->BindVertexStream(vertex_stream, { element });

			if (element.usage == Vertex::Position)
			{
				auto& ray_device = Context::Instance().GetRayContext().GetDevice();

				RayTracingGeometryInitializer initializer;

				initializer.Segement.VertexBuffer = vertex_stream.get();
				initializer.Segement.VertexFormat = element.format;
				initializer.Segement.VertexBufferStride = NumFormatBytes(element.format);
				initializer.Segement.NumPrimitives = asset.GetIndexCount() / 3;

				initializer.IndexBuffer = index_stream.get();

				tracing_geometry =white::unique_raw(ray_device.CreateRayTracingGeometry(initializer));
				tracing_geometry->BuildAccelerationStructure(*Context::Instance().GetDefaultCommandContext());

				min = white::math::float3(FLT_MAX,FLT_MAX,FLT_MAX);
				max = white::math::float3(FLT_MIN,FLT_MIN,FLT_MIN);
				auto stream =reinterpret_cast<white::math::float3*>(asset.GetVertexStreams()[i].get());
				for (uint32 i = 0; i != asset.GetVertexCount(); ++i)
				{
					min = white::math::min(stream[i], min);
					max = white::math::max(stream[i], max);
				}
			}
		}

		input_layout->BindIndexStream(index_stream, asset.GetIndexFormat());

		//Topo
		input_layout->SetTopoType(PrimtivteType::TriangleList);
	}
	const asset::MeshAsset::SubMeshDescrption::LodDescription & Mesh::GetSubMeshCurretnLodDescription(int submesh_index)
	{
		return GetSubMeshLodDescription(submesh_index, mesh_lod);
	}
	const asset::MeshAsset::SubMeshDescrption::LodDescription & Mesh::GetSubMeshLodDescription(int submesh_index, int lod_index)
	{
		return sub_meshes[submesh_index].LodsDescription[mesh_lod];
	}
	white::uint8 Mesh::GetSubMeshMaterialIndex(int submesh_index)
	{
		return sub_meshes[submesh_index].MaterialIndex;
	}

	const std::string& Mesh::GetName() const wnothrow {
		return name;
	}


	MeshesHolder::MeshesHolder()
		:loaded_meshes(&pool_resource)
	{
	}
	platform::MeshesHolder::~MeshesHolder()
	{
	}
	std::shared_ptr<void> platform::MeshesHolder::FindResource(const std::any & key)
	{
		if (auto ptuple = std::any_cast<std::tuple<std::shared_ptr<asset::MeshAsset>, std::string>>(&key))
			return FindResource(std::get<0>(*ptuple), std::get<1>(*ptuple));
		return {};
	}
	std::shared_ptr<Mesh> platform::MeshesHolder::FindResource(const std::shared_ptr<asset::MeshAsset>& asset, const std::string & name)
	{
		for (auto& pair : loaded_meshes)
		{
			if (auto sp = pair.first.lock())
				if (sp == asset && pair.second->GetName() == name)
					return pair.second;
		}
		return {};
	}
	void platform::MeshesHolder::Connect(const std::shared_ptr<asset::MeshAsset>& asset, const std::shared_ptr<Mesh>& mesh)
	{
		auto use_count = mesh.use_count();
		auto insert_itr = std::find_if(loaded_meshes.begin(), loaded_meshes.end(),[&](auto& pair) { return pair.second.use_count() == use_count; });
		loaded_meshes.emplace(insert_itr, asset, mesh);
		auto erase_count = 0;
		auto itr = loaded_meshes.rbegin();
		while (itr != loaded_meshes.rend() && itr->second.use_count() == 1)
			++erase_count,++itr;
		loaded_meshes.resize(loaded_meshes.size() - erase_count);
	}
	MeshesHolder & platform::MeshesHolder::Instance()
	{
		static MeshesHolder instance;
		return instance;
	}

	template<>
	std::shared_ptr<Mesh> AssetResourceScheduler::SyncSpawnResource<Mesh,const X::path&,const std::string&>(const X::path& path, const std::string & name) {
		auto pAsset = X::LoadMeshAsset(path);
		if (!pAsset)
			return {};
		if (auto pMesh = MeshesHolder::Instance().FindResource(pAsset,name))
			return pMesh;
		auto pMesh = std::make_shared<Mesh>(*pAsset,name);
		MeshesHolder::Instance().Connect(pAsset, pMesh);
		return pMesh;
	}

	template std::shared_ptr<Mesh> AssetResourceScheduler::SyncSpawnResource<Mesh, const X::path&, const std::string&>(const X::path& path, const std::string & name);

	white::coroutine::Task<std::shared_ptr<Mesh>> platform::X::AsyncLoadMesh(path const& meshpath, const std::string& name)
	{
		auto pAsset = co_await X::AsyncLoadMeshAsset(meshpath);

		co_await Environment->Scheduler->schedule_render();
		if (auto pMesh = MeshesHolder::Instance().FindResource(pAsset, name))
			co_return pMesh;

		auto pMesh = std::make_shared<Mesh>(*pAsset, name);

		MeshesHolder::Instance().Connect(pAsset, pMesh);

		co_await Environment->Scheduler->schedule();

		co_return pMesh;
	}

	white::coroutine::Task<void> platform::X::AsyncLoadMeshes(white::span<const X::path> pathes, white::span<std::shared_ptr<Mesh>> meshes)
	{
		std::vector<std::shared_ptr<asset::MeshAsset>> asset(pathes.size());

		co_await X::BatchLoadMeshAsset(pathes, white::make_span(asset));

		co_await Environment->Scheduler->schedule_render();

		for (size_t i = 0; i < pathes.size(); ++i)
		{
			auto name = X::path(pathes[i]).replace_extension().string();
			if (auto pMesh = MeshesHolder::Instance().FindResource(asset[i], name))
				meshes[i] = pMesh;
			else
			{
				pMesh = std::make_shared<Mesh>(*asset[i], name);
				MeshesHolder::Instance().Connect(asset[i], pMesh);
				meshes[i] = pMesh;
			}
		}

		co_return;
	}

}
