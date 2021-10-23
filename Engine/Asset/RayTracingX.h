#pragma once

#include <WBase/wmemory.hpp>
#include "../Render/IRayTracingShader.h"
#include "../Render/IRayDevice.h"
#include <filesystem>

namespace platform::X {
	using path = std::filesystem::path;

	std::shared_ptr<Render::RayTracingShader> LoadRayTracingShader(Render::RayDevice& Device, const path& filepath);
}