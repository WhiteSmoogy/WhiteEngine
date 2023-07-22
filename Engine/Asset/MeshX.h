/*! \file Engine\Asset\Mesh.h
\ingroup Engine
\brief Mesh IO ...
*/
#ifndef WE_ASSET_MESH_X_H
#define WE_ASSET_MESH_X_H 1


#include "Runtime/Renderer/Mesh.h"
#include <filesystem>
#include <string_view>
#include "Core/Coroutine/Task.h"
namespace platform {
	namespace X {
		using path = std::filesystem::path;

		std::shared_ptr<asset::MeshAsset> LoadMeshAsset(path const& meshpath);

		std::shared_ptr<Mesh> LoadMesh(path const& meshpath,const std::string& name);

		white::coroutine::Task<std::shared_ptr<asset::MeshAsset>> AsyncLoadMeshAsset(path const& meshpath);
		white::coroutine::Task<std::shared_ptr<Mesh>> AsyncLoadMesh(path const& meshpath, const std::string& name);


		white::coroutine::Task<void> BatchLoadMeshAsset(white::span<const path> pathes, white::span<std::shared_ptr<asset::MeshAsset>> asset);
		white::coroutine::Task<void> AsyncLoadMeshes(white::span<const path> pathes, white::span<std::shared_ptr<Mesh>> meshes);
	}
}
#endif