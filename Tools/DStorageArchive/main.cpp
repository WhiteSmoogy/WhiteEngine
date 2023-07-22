#include "DDSArchive.h"

#include "CLI11.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/wincolor_sink.h"

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

int main(int argc,char** argv)
{
    SetupLog();
    COM com;

    CLI::App app{ "Archive Resource to DStorage Load" };

    bool try_gdeflate = false;
    app.add_option("--gdeflate", try_gdeflate,"try compress by gpu deflate");

    std::vector<std::string> ddsfiles;
    app.add_option("--dds", ddsfiles, "dds file");

    uint64 stagingMB = DSFileFormat::kDefaultStagingBufferSize / 1024 / 1024;
    app.add_option("--stagingsize", stagingMB, "staging buffer size")->check(CLI::Range(16,256));

    std::filesystem::path extension = ".dds";

    app.add_option_function<std::vector<std::string>>("--ddsdir", [&](const std::vector<std::string>& ddsdirs) ->void
        {
            for (auto& dir : ddsdirs)
            {
                for (auto file : std::filesystem::directory_iterator(dir))
                {
                    if (file.path().extension() == extension)
                    {
                        auto ddsfile = file.path().string();
                        std::replace(ddsfile.begin(), ddsfile.end(), '\\', '/');
                        ddsfiles.emplace_back(ddsfile);
                    }
                }
            }
        }, "dds directory");

    std::string output = "dstorage.asset";

    app.add_option("--output", output, "output file path");

    CLI11_PARSE(app,argc,argv);

    if(try_gdeflate)
        CheckHResult(DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE, 0, IID_PPV_ARGS(g_gdeflate_codec.ReleaseAndGetAddress())));

    DDSArchive archive{stagingMB};

    archive.Archive(output, ddsfiles);
}