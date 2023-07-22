/*! \file Core\Mesh.h
\ingroup Engine
\brief 提供渲染所需的InputLayout。
*/
#ifndef WE_Core_Mesh_H
#define WE_Core_Mesh_H 1

#include "Asset/MeshAsset.h"
#include "Runtime/Resource.h"
#include "Runtime/ResourcesHolder.h"
#include "RenderInterface/InputLayout.hpp"
#include "RenderInterface/IRayTracingGeometry.h"

namespace platform {

	class Mesh :public Resource {
	public:
		Mesh(const asset::MeshAsset& asset, const std::string& name);

		const asset::MeshAsset::SubMeshDescrption::LodDescription& GetSubMeshCurretnLodDescription(int submesh_index);
		const asset::MeshAsset::SubMeshDescrption::LodDescription& GetSubMeshLodDescription(int submesh_index, int lod_index);
		white::uint8 GetSubMeshMaterialIndex(int submesh_index);

		DefGetter(const wnothrow, white::uint8, SubMeshCount, static_cast<white::uint8>(sub_meshes.size()))
			DefGetter(const wnothrow, const std::vector<asset::MeshAsset::SubMeshDescrption>&, SubMeshDesces, sub_meshes)
			DefGetter(const wnothrow, white::uint8, GeometryLod, mesh_lod)
			DefSetter(wnothrow, white::uint8, GeometryLod, mesh_lod)

			DefGetter(const wnothrow, const Render::InputLayout&, InputLayout, *input_layout);
		DefGetter(wnothrow, Render::RayTracingGeometry*, RayTracingGeometry, tracing_geometry.get());

		white::math::float3 GetBoundingMin() const { return min; }
		white::math::float3 GetBoundingMax() const { return max; }

		const std::string& GetName() const wnothrow;
	private:
		std::unique_ptr<Render::InputLayout> input_layout;
		std::vector<asset::MeshAsset::SubMeshDescrption> sub_meshes;
		std::unique_ptr<Render::RayTracingGeometry> tracing_geometry;
		white::uint8 mesh_lod = 0;
		std::string name;

		white::math::float3 min;
		white::math::float3 max;
	};

	class MeshesHolder :ResourcesHolder<Mesh> {
	public:
		MeshesHolder();
		~MeshesHolder();

		std::shared_ptr<void> FindResource(const std::any& key) override;
		std::shared_ptr<Mesh> FindResource(const std::shared_ptr<asset::MeshAsset>& asset, const std::string& name);

		void Connect(const std::shared_ptr<asset::MeshAsset>& asset, const std::shared_ptr<Mesh>& mesh);

		static MeshesHolder& Instance();
	private:
		std::pmr::vector<std::pair<std::weak_ptr<asset::MeshAsset>, std::shared_ptr<Mesh>>> loaded_meshes;
	};

}

#endif