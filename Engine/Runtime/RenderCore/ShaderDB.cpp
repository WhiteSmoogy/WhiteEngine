#include "ShaderDB.h"
#include "Runtime/Path.h"
#define SQLITE_ORM_OMITS_CODECVT 1
#include <sqlite/sqlite_orm.h>
#include <mutex>
#include <shared_mutex>
#include <WBase/algorithm.hpp>
#include "System/SystemEnvironment.h"
#include "spdlog/spdlog.h"

using namespace WhiteEngine;
using namespace sqlite_orm;
namespace fs = std::filesystem;

struct FileTime
{
	std::string FileName;
	long long TimePoint;
};

struct FileDependent
{
	std::string FileName;

	std::string Dependents;
};

const fs::path& StoragePath() {
	auto static path = (PathSet::EngineIntermediateDir() / "Shaders.db");
	return path;
}

std::shared_mutex mutex;

decltype(auto) Storage()
{
	static std::once_flag init_db;
	
	auto make_storage = [&]()
	{
		return sqlite_orm::make_storage(StoragePath().string(),
			make_table("filetimes",
				make_column("filename", &FileTime::FileName, primary_key()),
				make_column("filetime", &FileTime::TimePoint)
			),
			make_table("filedependent",
				make_column("filename", &FileDependent::FileName, primary_key()),
				make_column("dependents", &FileDependent::Dependents)
			)
		);
	};

	std::call_once(init_db, [&] {
			make_storage().sync_schema();
		});

	thread_local static auto& storage = [&]() -> decltype(auto) {
		thread_local static auto internal_storage = make_storage();

		return (internal_storage);
	}();

	return storage;
}

std::optional<fs::file_time_type> WhiteEngine::ShaderDB::QueryTime(const std::string& path)
{
	std::shared_lock lock{ mutex };
	auto& storage = Storage();

	if (auto filetime = storage.get<FileTime>(path))
		return std::filesystem::file_time_type(std::chrono::file_clock::duration(filetime->TimePoint));
	else
		spdlog::debug("QueryTime sqlite3 {} {}",path,filetime.error());

	return std::nullopt;
}

std::optional<std::vector<std::string>> WhiteEngine::ShaderDB::QueryDependent(const std::string& key)
{
	std::shared_lock lock{ mutex };
	auto& storage = Storage();

	if (auto filedependent = storage.get<FileDependent>(key))
	{
		std::vector<std::string> dependents;

		white::split(filedependent->Dependents.begin(), filedependent->Dependents.end(), [](char c)
			{
				return c == ',';
			},
			[&](auto i, auto b) {
				dependents.emplace_back(std::string(i, b));
			}
		);
		return dependents;
	}
	else
		spdlog::debug("QueryDependent sqlite3 {}", filedependent.error());

	return std::nullopt;
}

white::coroutine::Task<void> WhiteEngine::ShaderDB::UpdateTime(const std::string& path, fs::file_time_type time)
{
	std::unique_lock lock{ mutex };
	auto& storage = Storage();

	if (auto unit = storage.replace(FileTime{ .FileName = path,.TimePoint = time.time_since_epoch().count() }); !unit)
	{
		spdlog::debug("UpdateTime sqlite3 {}", unit.error());
	}
	co_return;
}

white::coroutine::Task<void> WhiteEngine::ShaderDB::UpdateDependent(const std::string& key, const std::vector<std::string>& dependents)
{
	std::unique_lock lock{ mutex };
	auto& storage = Storage();
	std::string dependent;

	for (std::size_t i = dependents.size(); i > 1; --i)
	{
		dependent += dependents[i-1];
		dependent += ",";
	}

	if (!dependent.empty())
		dependent += dependents[0];

	if (auto unit = storage.replace(FileDependent{ .FileName = key,.Dependents = dependent }); !unit)
	{
		spdlog::debug("UpdateDependent sqlite3 {}", unit.error());
	}

	co_return;
}
