#pragma once

#include <WBase/wmemory.hpp>
#include "RenderInterface/IRayTracingShader.h"
#include "RenderInterface/IRayDevice.h"
#include <filesystem>

namespace platform::X {
	using path = std::filesystem::path;

	std::shared_ptr<Render::RayTracingShader> LoadRayTracingShader(Render::RayDevice& Device, const path& filepath);
}