#include <WBase/scope_gurad.hpp>

#include "Initialization.h"
#include "WFramework/WCLib/Platform.h"
#include "WFramework/Win32/WCLib/NLS.h"
#include "WFramework/Service/Filesystem.h"
#include "WFramework/Adaptor/WAdaptor.h"
#include "WFramework/Service/TextFile.h"

#include <WScheme/Configuration.h>

using namespace Test;
using namespace platform_ex;
using namespace scheme;

namespace white {
	using namespace IO;
	namespace {
#undef CONF_PATH
#undef DATA_DIRECTORY
#undef DEF_FONT_DIRECTORY
#undef DEF_FONT_PATH
#if WFL_Win32 || WFL_Linux
		const string&
			FetchWorkingRoot()
		{
			static const struct Init
			{
				string Path;

				Init()
					: Path([] {
#	if WFL_Win32
					IO::Path image(platform::ucast(
						platform_ex::Windows::FetchModuleFileName().data()));

					if (!image.empty())
					{
						image.pop_back();

						const auto& dir(image.Verify());

						if (!dir.empty() && dir.back() == FetchSeparator<char16_t>())
							return dir.GetMBCS();
					}
#	elif WFL_Android
					const char*
						sd_paths[]{ "/sdcard/", "/mnt/sdcard/", "/storage/sdcard0/" };

					for (const auto& path : sd_paths)
						if (IO::VerifyDirectory(path))
						{
							YTraceDe(Informative, "Successfully found SD card path"
								" '%s' as root path.", path);
							return path;
						}
						else
							YTraceDe(Informative,
								"Failed accessing SD card path '%s'.", path);
#	elif WFL_Linux
					// FIXME: What if link reading failed (i.e. permission denied)?
					// XXX: Link content like 'node_type:[inode]' is not supported.
					// TODO: Use implemnetation for BSD family OS, etc.
					auto image(IO::ResolvePath<white::path<vector<string>,
						IO::PathTraits>>(string_view("/proc/self/exe")));

					if (!image.empty())
					{
						image.pop_back();

						const auto& dir(IO::VerifyDirectoryPathTail(
							white::to_string_d(image)));

						if (!dir.empty() && dir.back() == FetchSeparator<char>())
							return dir;
					}
#	else
#		error "Unsupported platform found."
#	endif
					throw GeneralEvent("Failed finding working root path.");
				}())
				{
					TraceDe(Informative, "Initialized root directory path '%s'.",
						Path.c_str());
				}
			} init;

			return init.Path;
		}

		// TODO: Reduce overhead?
#	define CONF_PATH (FetchWorkingRoot() + "wconf.wsl").c_str()
#endif
#if WFL_DS
#	define DATA_DIRECTORY "/Data/"
#	define DEF_FONT_DIRECTORY "/Font/"
#	define DEF_FONT_PATH "/Font/FZYTK.TTF"
#elif WFL_Win32
#	define DATA_DIRECTORY FetchWorkingRoot()
#	define DEF_FONT_PATH (FetchSystemFontDirectory_Win32() + "SimSun.ttc")
	//! \since build 693
		inline PDefH(string, FetchSystemFontDirectory_Win32, )
			// NOTE: Hard-coded as Shell32 special path with %CSIDL_FONTS or
			//	%CSIDL_FONTS. See https://msdn.microsoft.com/en-us/library/dd378457.aspx.
			ImplRet(Windows::WCSToUTF8(Windows::FetchWindowsPath()) + "Fonts\\")
#elif WFL_Android
#	define DATA_DIRECTORY (FetchWorkingRoot() + "Data/")
#	define DEF_FONT_DIRECTORY "/system/fonts/"
#	define DEF_FONT_PATH "/system/fonts/DroidSansFallback.ttf"
#elif WFL_Linux
#	define DATA_DIRECTORY FetchWorkingRoot()
#	define DEF_FONT_PATH "./SimSun.ttc"
#else
#	error "Unsupported platform found."
#endif
#ifndef CONF_PATH
#	define CONF_PATH "yconf.txt"
#endif
#ifndef DATA_DIRECTORY
#	define DATA_DIRECTORY "./"
#endif
#ifndef DEF_FONT_DIRECTORY
#	define DEF_FONT_DIRECTORY DATA_DIRECTORY
#endif
	}

}

namespace Test {
	using namespace white;

	white::ValueNode&
		FetchRoot() wnothrow {
		static ValueNode Root = LoadConfiguration(true);
		if (Root.GetName() == "WFramework")
			Root = PackNodes(string(), std::move(Root));
		WF_Trace(Debug, "Root lifetime began.");
		return Root;
	}

	void
		WriteWSLA1Stream(std::ostream& os, scheme::Configuration&& conf)
	{
		white::write_literal(os, Text::BOM_UTF_8) << std::move(conf);
	}

	ValueNode
		TryReadRawNPLStream(std::istream& is)
	{
		scheme::Configuration conf;

		is >> conf;
		TraceDe(Debug, "Plain configuration loaded.");
		if (!conf.GetNodeRRef().empty())
			return conf.GetNodeRRef();
		TraceDe(Warning, "Empty configuration found.");
		throw GeneralEvent("Invalid stream found when reading configuration.");
	}

	WB_NONNULL(1, 2)white::ValueNode LoadWSLV1File(const char * disp, const char * path, white::ValueNode(*creator)(), bool show_info)
	{
		auto res(TryInvoke([=]() -> ValueNode {
			if (!ufexists(path))
			{
				TraceDe(Debug, "Path '%s' access failed.", path);
				if (show_info)
					TraceDe(Notice, "Creating %s '%s'...", disp, path);

				swap_guard<int, void, decltype(errno)&> gd(errno, 0);

				// XXX: Failed on race condition detected.
				if (UniqueLockedOutputFileStream uofs{ std::ios_base::out
					| std::ios_base::trunc | platform::ios_noreplace, path })
					WriteWSLA1Stream(uofs, Nonnull(creator)());
				else
				{
					TraceDe(Warning, "Cannot create file, possible error"
						" (from errno) = %d: %s.", errno, std::strerror(errno));
					TraceDe(Warning, "Creating default file failed.");
					return {};
				}
				TraceDe(Debug, "Created configuration.");
			}
			if (show_info)
				TraceDe(Notice, "Found %s '%s'.", Nonnull(disp), path);
			// XXX: Race condition may cause failure, though file would not be
			//	corrupted now.
			if (SharedInputMappedFileStream sifs{ path })
			{
				TraceDe(Debug, "Accessible configuration file found.");
				if (Text::CheckBOM(sifs, Text::BOM_UTF_8))
					return TryReadRawNPLStream(sifs);
				TraceDe(Warning, "Wrong encoding of configuration file found.");
			}
			TraceDe(Err, "Configuration corrupted.");
			return {};
		}));

		if (res)
			return res;
		TraceDe(Notice, "Trying fallback in memory...");

		std::stringstream ss;

		ss << Nonnull(creator)();
		return TryReadRawNPLStream(ss);
	}

	white::ValueNode LoadConfiguration(bool show_info)
	{
		return LoadWSLV1File("configuration file", CONF_PATH, [] {
			return white::ValueNode(white::NodeLiteral{ "WFramework",
			{ { "DataDirectory", DATA_DIRECTORY },{ "FontFile", DEF_FONT_PATH },
			{ "FontDirectory", DEF_FONT_DIRECTORY } } });
		}, show_info);
	}
}

