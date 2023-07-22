#pragma once

#include "Engine/Asset/MaterialX.h"
#include "Runtime/Renderer/Mesh.h"
#include "WBase/wmathtype.hpp"
#include "Engine/Asset/MeshX.h"
#include "RenderInterface/IRayTracingScene.h"
#include "RenderInterface/IRayDevice.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/IRayContext.h"
#include "WBase/span.hpp"

#include <string>
#include <filesystem>
#include <vector>
#include <memory>

namespace fs = std::filesystem;

class Entity {
public:
	Entity(std::shared_ptr<platform::Mesh> mesh,const std::string& material_name) 
	:pMesh(mesh){
		pMaterial = platform::X::LoadMaterial(material_name + ".mat.wsl", material_name);
	}

	const platform::Material& GetMaterial() const {
		return *pMaterial;
	}

	platform::Mesh& GetMesh() {
		return *pMesh;
	}

	const platform::Mesh& GetMesh() const {
		return *pMesh;
	}
private:
	std::shared_ptr<platform::Mesh> pMesh;
	std::shared_ptr<platform::Material> pMaterial;
};

class Entities {
public:
	Entities(const fs::path& file);

	const std::vector<Entity>& GetRenderables() const {
		return entities;
	}

	white::shared_ptr<platform::Render::RayTracingScene> CreateRayTracingScene()
	{
		platform::Render::RayTracingSceneInitializer initializer;

		std::vector<platform::Render::RayTracingGeometryInstance> Instances;

		for (auto& entity : entities)
		{
			platform::Render::RayTracingGeometryInstance Instance;
			Instance.Geometry = entity.GetMesh().GetRayTracingGeometry();

			Instance.Transform = white::math::float4x4::identity;

			Instances.push_back(Instance);
		}

		initializer.Instances = white::make_span(Instances);

		return white::share_raw(platform::Render::Context::Instance().GetRayContext().GetDevice().CreateRayTracingScene(initializer));
	}

	white::math::float3 min;
	white::math::float3 max;
private:
	std::vector<Entity> entities;
};