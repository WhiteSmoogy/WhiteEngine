#include "Entities.h"
#include "Asset/WSLAssetX.h"
#include "System/SystemEnvironment.h"
#include "Runtime/Core/Coroutine/WhenAllReady.h"
#include "Runtime/Core/Coroutine/SyncWait.h"
#include "spdlog/spdlog.h"

#include <WFramework/Helper/ShellHelper.h>

std::string Access(const char* name, const scheme::TermNode& node) {
	auto it = std::find_if(node.begin(), node.end(), [&](const scheme::TermNode& child) {
		if (!child.empty())
			return white::Access<std::string>(*child.begin()) == name;
		return false;
		});
	return white::Access<std::string>(*(it->rbegin()));
}

Entities::Entities(const fs::path& file) {
	LOG_TRACE(__FUNCTION__);

	auto term_node = *platform::X::LoadNode(file).begin();

	auto entity_nodes = platform::X::SelectNodes("entity", term_node);
	
	std::vector<std::shared_ptr<platform::Mesh>> meshs;
	std::vector<platform::X::path> pathes;

	meshs.resize(entity_nodes.size());
	for (auto& entity_node : entity_nodes)
	{
		auto mesh_name = Access("mesh", entity_node);
		pathes.emplace_back(mesh_name + ".asset");
	}

	white::coroutine::SyncWait(platform::X::AsyncLoadMeshes(white::make_const_span(pathes),white::make_span(meshs)));

	int index = 0;
	for (auto& entity_node : platform::X::SelectNodes("entity", term_node))
	{
		auto material_name = Access("material", entity_node);
		entities.emplace_back(meshs[index++], material_name);
	}

	min = white::math::float3(FLT_MAX, FLT_MAX, FLT_MAX);
	max = white::math::float3(FLT_MIN, FLT_MIN, FLT_MIN);
	for (auto& entity : entities)
	{
		min = white::math::min(min, entity.GetMesh().GetBoundingMin());
		max = white::math::max(max, entity.GetMesh().GetBoundingMax());
	}
}