#pragma once

#include "Core/Coroutine/Task.h"

#include <chrono>
#include <optional>
#include <filesystem>
#include <string>
namespace WhiteEngine
{
	class ShaderDB {
	public:
		static std::optional<std::filesystem::file_time_type> QueryTime(const std::string& key);
		static std::optional<std::vector<std::string>> QueryDependent(const std::string& key);

		static white::coroutine::Task<void> UpdateTime(const std::string& key, std::filesystem::file_time_type time);
		static white::coroutine::Task<void> UpdateDependent(const std::string& key,const std::vector<std::string>& time);
	};
}