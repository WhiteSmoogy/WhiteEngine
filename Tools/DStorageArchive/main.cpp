#include "DDSArchive.h"
#include "TrinfArchive.h"
#include "CLI11.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/wincolor_sink.h"

#include <regex>

using namespace platform_ex;
using namespace WhiteEngine;




COMPtr<IDStorageCompressionCodec> g_gdeflate_codec;

void SetupLog()
{
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Archive.log", true);
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();

    spdlog::set_default_logger(white::share_raw(new spdlog::logger("spdlog", 
        { 
            file_sink,
            msvc_sink,
            color_sink
        })));
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    file_sink->set_level(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(5));
    spdlog::flush_on(spdlog::level::warn);
    file_sink->set_pattern("[%H:%M:%S:%e][%^%l%$][thread %t] %v");
    msvc_sink->set_pattern("[thread %t] %v");

    white::FetchCommonLogger().SetSender([=](platform::Descriptions::RecordLevel lv, platform::Logger& logger, const char* str) {
        switch (lv)
        {
        case platform::Descriptions::Emergent:
        case platform::Descriptions::Alert:
        case platform::Descriptions::Critical:
            spdlog::critical(str);
            break;
        case platform::Descriptions::Err:
            spdlog::error(str);
            break;
        case platform::Descriptions::Warning:
            spdlog::warn(str);
            break;
        case platform::Descriptions::Notice:
            file_sink->flush();
        case platform::Descriptions::Informative:
            spdlog::info(str);
            break;
        case platform::Descriptions::Debug:
            spdlog::trace(str);
            break;
        }
        }
    );
}

enum class Format
{
    Textures,
    Trinf,
};

int main(int argc,char** argv)
{
    SetupLog();
    COM com;

    CLI::App app{ "Archive Resource to DStorage Load" };

    bool try_gdeflate = false;
    app.add_option("--gdeflate", try_gdeflate,"try compress by gpu deflate");

    std::vector<std::string> files;
    app.add_option("--file", files, "input files");

    uint64 stagingMB = DSFileFormat::kDefaultStagingBufferSize / 1024 / 1024;
    app.add_option("--stagingsize", stagingMB, "staging buffer size")->check(CLI::Range(16,256));


    std::string filter = "";
    app.add_option("--filter", filter, "regex filter file");

    Format format{ Format::Textures };
    // specify string->value mappings
    std::map<std::string, Format> map{ {"tex",Format::Textures}, {"tri", Format::Trinf}};
    app.add_option("-f,--format", format, "format settings")
        ->required()->transform(CLI::CheckedTransformer(map, CLI::ignore_case));

    app.add_option_function<std::vector<std::string>>("--dir", [&](const std::vector<std::string>& dirs) ->void
        {
            for (auto& dir : dirs)
            {
                for (auto file : std::filesystem::directory_iterator(dir))
                {
                        auto normalize_name = file.path().string();
                        std::replace(normalize_name.begin(), normalize_name.end(), '\\', '/');
                        files.emplace_back(normalize_name);
                }
            }
        }, "input directory");

    std::string output = "dstorage.asset";

    app.add_option("--output", output, "output file path")->required();

    CLI11_PARSE(app,argc,argv);

    std::vector<std::string> filterfiles;

    if (filter.size() > 0)
    {
        std::regex pattern(filter);
        for (auto& file : files)
        {
            if (std::regex_search(file, pattern))
                filterfiles.emplace_back(file);
        }

        filterfiles.swap(files);
    }
    

    if(try_gdeflate)
        CheckHResult(DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE, 0, IID_PPV_ARGS(g_gdeflate_codec.ReleaseAndGetAddress())));

    static auto pInitGuard = WhiteEngine::System::InitGlobalEnvironment();

    if (format == Format::Textures)
    {
        DDSArchive archive{ stagingMB };

        archive.Archive(output, files);
    }
    else
    {
        TrinfArchive archive{ stagingMB };

        archive.Archive(output, files);
    }
}