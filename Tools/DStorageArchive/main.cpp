#include <Asset/DStorageAsset.h>
#include <WFramework/Win32/WCLib/COM.h>
#include <D3D12/d3d12_dxgi.h>
#include <Asset/Loader.hpp>
#include <dstorage.h>
#include <dstorageerr.h>
#include <Runtime/Core/Serialization/MemoryWriter.h>
#include <Asset/TextureX.h>
#include <Asset/DDSX.h>
#include "CLI11.hpp"
#include <filesystem>
#include <iostream>
#include <zlib.h>
#include <ranges>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/wincolor_sink.h"

using namespace platform_ex;
using namespace WhiteEngine;
static COMPtr<IDStorageCompressionCodec> g_gdeflate_codec;

template<typename T>
static DStorageCompressionFormat BestCompressFormat(T&& source)
{
    DStorageCompressionFormat compression = DStorageCompressionFormat::None;

    size_t bestSize = source.size();
    
    size_t gdeflatSize = g_gdeflate_codec->CompressBufferBound(static_cast<uint32_t>(source.size()));

    std::remove_reference_t<T> dest;
    dest.resize(gdeflatSize);

    size_t actualCompressedSize = 0;
    auto compressionResult = g_gdeflate_codec->CompressBuffer(
        reinterpret_cast<const void*>(source.data()),
        static_cast<uint32_t>(source.size()),
        DSTORAGE_COMPRESSION_BEST_RATIO,
        reinterpret_cast<void*>(dest.data()),
        static_cast<uint32_t>(dest.size()),
        &actualCompressedSize);

    if (SUCCEEDED(compressionResult) && actualCompressedSize < bestSize)
    {
        bestSize = actualCompressedSize;
        compression = DStorageCompressionFormat::GDeflate;
    }

    size_t zlibSize = static_cast<size_t>(compressBound(static_cast<uLong>(source.size())));
    dest.resize(zlibSize);

    uLong destSize = static_cast<uLong>(dest.size());
    int result = compress(
        reinterpret_cast<Bytef*>(dest.data()),
        &destSize,
        reinterpret_cast<Bytef const*>(source.data()),
        static_cast<uLong>(source.size()));

    if (result == Z_OK && destSize < bestSize)
    {
        bestSize = destSize;
        compression = DStorageCompressionFormat::Zlib;
    }

    return compression;
}

template<typename T>
static std::remove_reference_t<T> Compress(DStorageCompressionFormat compression, T&& source)
{
    if (compression == DStorageCompressionFormat::None)
    {
        return source;
    }
    else
    {
        size_t maxSize;
        if (compression == DStorageCompressionFormat::GDeflate)
            maxSize = g_gdeflate_codec->CompressBufferBound(static_cast<uint32_t>(source.size()));
        else if (compression == DStorageCompressionFormat::Zlib)
            maxSize = static_cast<size_t>(compressBound(static_cast<uLong>(source.size())));
        else
            throw std::runtime_error("Unknown Compression type");

        std::remove_reference_t<T> dest;
        dest.resize(maxSize);

        size_t actualCompressedSize = 0;

        HRESULT compressionResult = S_OK;

        if (compression == DStorageCompressionFormat::GDeflate)
        {
            compressionResult = g_gdeflate_codec->CompressBuffer(
                reinterpret_cast<const void*>(source.data()),
                static_cast<uint32_t>(source.size()),
                DSTORAGE_COMPRESSION_BEST_RATIO,
                reinterpret_cast<void*>(dest.data()),
                static_cast<uint32_t>(dest.size()),
                &actualCompressedSize);
        }
        else if (compression == DStorageCompressionFormat::Zlib)
        {
            uLong destSize = static_cast<uLong>(dest.size());
            int result = compress(
                reinterpret_cast<Bytef*>(dest.data()),
                &destSize,
                reinterpret_cast<Bytef const*>(source.data()),
                static_cast<uLong>(source.size()));

            if (result == Z_OK)
                compressionResult = S_OK;
            else
                compressionResult = E_FAIL;

            actualCompressedSize = destSize;
        }

        if (FAILED(compressionResult))
        {
            std::cout << "Failed to compress data using CompressBuffer, hr = 0x" << std::hex << compressionResult
                << std::endl;
            std::abort();
        }

        dest.resize(actualCompressedSize);

        return dest;
    }
}

template<typename T>
void WriteArray(std::ostream& s, T const* data, size_t count)
{
    s.write((char*)data, sizeof(T) * count);
}

template<typename T>
void WriteArray(Archive& s, T const* data, size_t count)
{
    s.Serialize(const_cast<T*>(data), sizeof(T) * count);
}

template<typename CONTAINER>
DSFileFormat::OffsetArray<typename CONTAINER::value_type> WriteArray(std::ostream& s, CONTAINER const& data)
{
    auto pos = s.tellp();
    WriteArray(s, data.data(), data.size());

    DSFileFormat::OffsetArray<typename CONTAINER::value_type> array;
    array.Offset = static_cast<uint32_t>(pos);

    return array;
}

template<typename CONTAINER>
DSFileFormat::OffsetArray<typename CONTAINER::value_type> WriteArray(Archive& s, CONTAINER const& data)
{
    auto pos = s.Tell();
    WriteArray(s, data.data(), data.size());

    DSFileFormat::OffsetArray<typename CONTAINER::value_type> array;
    array.Offset = static_cast<uint32_t>(pos);

    return array;
}

//TODO:RegionWriteStream
void Patch(std::ostream& s, std::streampos pos, void const* data, size_t size)
{
    auto oldPos = s.tellp();
    s.seekp(pos);
    s.write(reinterpret_cast<char const*>(data), size);
    s.seekp(oldPos);
}

void Patch(Archive& s, std::streampos pos, void const* data, size_t size)
{
    auto oldPos = s.Tell();
    s.Seek(pos);
    s.Serialize(const_cast<void*>(data), size);
    s.Seek(oldPos);
}

template<typename T>
void Patch(std::ostream& s, std::streampos pos, T const& value)
{
    Patch(s, pos, &value, sizeof(value));
}

template<typename T>
void Patch(Archive& s, std::streampos pos, T const& value)
{
    Patch(s, pos, &value, sizeof(value));
}

template<typename T>
void Patch(std::ostream& s, std::streampos pos, std::vector<T> const& values)
{
    Patch(s, pos, values.data(), values.size() * sizeof(T));
}

template<typename T>
class Fixup
{
    std::streampos m_pos;

public:
    Fixup() = default;

    Fixup(std::streampos fixupPos)
        : m_pos(fixupPos)
    {
    }

    void Set(std::ostream& stream, T const& value) const
    {
        Patch(stream, m_pos, value);
    }

    void Set(Archive& stream, T const& value) const
    {
        Patch(stream, m_pos, value);
    }

    template<typename Y>
    void FixOffset(T* const value, DSFileFormat::OffsetPtr<Y>& field)
    {
        auto byteSrc = reinterpret_cast<uint8_t const*>(value);
        auto byteField = reinterpret_cast<uint8_t const*>(&field);
        std::ptrdiff_t offset = byteField - byteSrc;

        if ((offset + sizeof(DSFileFormat::OffsetPtr<Y>)) > sizeof(T))
        {
            throw std::runtime_error("FixOffset outside of src structure");
        }

        field.Offset =static_cast<uint32>(field.Offset - m_pos - offset);
    }
};

template<typename T, typename FIXUP>
Fixup<FIXUP> MakeFixup(std::streampos startPos, T const* src, FIXUP const* fixup)
{
    auto byteSrc = reinterpret_cast<uint8_t const*>(src);
    auto byteField = reinterpret_cast<uint8_t const*>(fixup);
    std::ptrdiff_t offset = byteField - byteSrc;

    if ((offset + sizeof(*fixup)) > sizeof(*src))
    {
        throw std::runtime_error("Fixup outside of src structure");
    }

    std::streampos fixupPos = startPos + offset;

    return Fixup<FIXUP>(fixupPos);
}

template<typename T, typename... FIXUPS>
std::tuple<Fixup<FIXUPS>...> WriteStruct(std::ostream& out, T const* src, FIXUPS const*... fixups)
{
    auto startPos = out.tellp();
    out.write(reinterpret_cast<char const*>(src), sizeof(*src));

    return std::make_tuple(MakeFixup(startPos, src, fixups)...);
}

template<typename T, typename... FIXUPS>
std::tuple<Fixup<FIXUPS>...> WriteStruct(Archive& out, T const* src, FIXUPS const*... fixups)
{
    auto startPos = out.Tell();
    out.Serialize(const_cast<T*>(src), sizeof(*src));

    return std::make_tuple(MakeFixup(startPos, src, fixups)...);
}

static_assert(sizeof(D3D12_RESOURCE_DESC) == sizeof(DSFileFormat::D3DResourceDesc));
static_assert(offsetof(D3D12_RESOURCE_DESC, Dimension) == offsetof(DSFileFormat::D3DResourceDesc, Dimension));
static_assert(offsetof(D3D12_RESOURCE_DESC, Alignment) == offsetof(DSFileFormat::D3DResourceDesc, Alignment));
static_assert(offsetof(D3D12_RESOURCE_DESC, Width) == offsetof(DSFileFormat::D3DResourceDesc, Width));
static_assert(offsetof(D3D12_RESOURCE_DESC, Height) == offsetof(DSFileFormat::D3DResourceDesc, Height));
static_assert(offsetof(D3D12_RESOURCE_DESC, DepthOrArraySize) == offsetof(DSFileFormat::D3DResourceDesc, DepthOrArraySize));
static_assert(offsetof(D3D12_RESOURCE_DESC, MipLevels) == offsetof(DSFileFormat::D3DResourceDesc, MipLevels));
static_assert(offsetof(D3D12_RESOURCE_DESC, Format) == offsetof(DSFileFormat::D3DResourceDesc, Format));
static_assert(offsetof(D3D12_RESOURCE_DESC, SampleDesc) == offsetof(DSFileFormat::D3DResourceDesc, SampleDesc));
static_assert(offsetof(D3D12_RESOURCE_DESC, Layout) == offsetof(DSFileFormat::D3DResourceDesc, Layout));
static_assert(offsetof(D3D12_RESOURCE_DESC, Flags) == offsetof(DSFileFormat::D3DResourceDesc, Flags));

class DDSArchive
{
public:
    DDSArchive(uint64 stagingBufferMb)
        :stagingBufferSize(stagingBufferMb << 20)
    {
        CheckHResult(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.ReleaseAndGetAddress())));
    }

    void Archive(const std::string& output,const std::vector<std::string>& ddsfiles)
    {
        out.open(output, std::ios::out | std::ios::trunc | std::ios::binary);

        DSFileFormat::Header header;
        header.Signature = asset::four_cc_v<'D', 'S', 'F', 'F'>;
        header.Version = 0;

        //placeholder
        auto [fixHeader] = WriteStruct(out, &header, &header);

        WriteTextures(ddsfiles);
        header.CpuMedadata = WriteCpuMetadata(ddsfiles);

        fixHeader.Set(out, header);

        spdlog::info("Archived {} {} MB", output, out.tellp() >> 20);
    }

    ~DDSArchive()
    {
        out.close();
    }
private:
    void  WriteTextures(const std::vector<std::string>& ddsfiles)
    {
        for (auto& ddsfile : ddsfiles)
        {
            platform::X::path path{ ddsfile };

            platform::File file(path.wstring(), platform::File::kToRead);
            platform::Render::TextureType in_type;
            uint16_t in_width, in_height, in_depth;
            uint8_t in_num_mipmaps;
            uint8_t in_array_size;
            platform::Render::EFormat in_format;
            std::vector<platform::Render::ElementInitData> in_data;
            std::vector<uint8_t> in_data_block;
            platform::X::GetImageInfo(file, in_type, in_width, in_height, in_depth, in_num_mipmaps, in_array_size, in_format, in_data, in_data_block);

            platform_ex::Windows::D3D12::CD3DX12_RESOURCE_DESC desc{};
            desc.Width = in_width;
            desc.Height = in_height;
            desc.MipLevels = in_num_mipmaps;
            desc.DepthOrArraySize = (in_type == platform::Render::TextureType::T_3D) ? in_depth : in_array_size;

            desc.Format = dds::ToDXGIFormat(in_format);
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;

            desc.Dimension = (D3D12_RESOURCE_DIMENSION)in_type;

            auto const subresourceCount = desc.Subresources(device.Get());

            std::vector<DSFileFormat::GpuRegion> regions;
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(subresourceCount);
            std::vector<UINT> numRows(subresourceCount);
            std::vector<UINT64> rowSizes(subresourceCount);

            uint64_t totalBytes = 0;
            uint32_t currentSubresource = 0;

            device->GetCopyableFootprints(&desc,
                currentSubresource,
                subresourceCount - currentSubresource,
                0,
                layouts.data(),
                numRows.data(),
                rowSizes.data(),
                &totalBytes);

            //separate mips as single region until remaining totalBytes <= stagingBuffer
            while (totalBytes > stagingBufferSize)
            {
                device->GetCopyableFootprints(&desc,
                    currentSubresource,
                    1,
                    0,
                    layouts.data(),
                    numRows.data(),
                    rowSizes.data(),
                    &totalBytes);

                if (totalBytes > stagingBufferSize)
                {
                    spdlog::error("Mip {} won't fit in the staging buffer.", currentSubresource);
                    spdlog::error("Try add --stagingsize={} to the command-line",(totalBytes + 1024 * 1024 - 1) / 1024 / 1024);
                    std::exit(1);
                }

                auto regionName = std::format("{} mip {}", ddsfile, currentSubresource);

                regions.push_back(WriteTextureRegion(
                    currentSubresource,
                    1,
                    layouts,
                    numRows,
                    rowSizes,
                    totalBytes,
                    in_data,
                    regionName));

                ++currentSubresource;

                if (currentSubresource < subresourceCount)
                {
                    //remaining size
                    device->GetCopyableFootprints(
                        &desc,
                        currentSubresource,
                        subresourceCount - currentSubresource,
                        0,
                        layouts.data(),
                        numRows.data(),
                        rowSizes.data(),
                        &totalBytes);
                }
            }

            DSFileFormat::GpuRegion remainingMipsRegion{};

            if (currentSubresource < subresourceCount)
            {
                auto regionName = std::format("{} mips {} to {}", ddsfile, currentSubresource, subresourceCount);
                remainingMipsRegion = WriteTextureRegion(
                    currentSubresource,
                    subresourceCount - currentSubresource,
                    layouts,
                    numRows,
                    rowSizes,
                    totalBytes,
                    in_data,
                    regionName);
            }

            TextureMetadata textureMetadata;
            textureMetadata.SingleMips = std::move(regions);
            textureMetadata.RemainingMips = remainingMipsRegion;

            texture_metadatas.emplace_back(std::move(textureMetadata));
            texture_descs.emplace_back(*reinterpret_cast<DSFileFormat::D3DResourceDesc*>(&desc));
        }
    }

    DSFileFormat::GpuRegion WriteTextureRegion(
        uint32_t currentSubresource,
        uint32_t numSubresources,
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> const& layouts,
        std::vector<UINT> const& numRows,
        std::vector<UINT64> const& rowSizes,
        uint64_t totalBytes,
        std::vector<platform::Render::ElementInitData> const& subresources,
        std::string const& name)
    {
        std::vector<white::byte> data(totalBytes);

        for (auto i = 0u; i < numSubresources; ++i)
        {
            auto const& layout = layouts[i];

            D3D12_MEMCPY_DEST memcpyDest{};
            memcpyDest.pData = data.data() + layout.Offset;
            memcpyDest.RowPitch = layout.Footprint.RowPitch;
            memcpyDest.SlicePitch = layout.Footprint.RowPitch * numRows[i];

            D3D12_SUBRESOURCE_DATA subresource;
            subresource.pData = subresources[currentSubresource + i].data;
            subresource.RowPitch = subresources[currentSubresource + i].row_pitch;
            subresource.SlicePitch = subresources[currentSubresource + i].slice_pitch;

            platform_ex::Windows::D3D12::MemcpySubresource(
                &memcpyDest,
                &subresource,
                static_cast<SIZE_T>(rowSizes[i]),
                numRows[i],
                layout.Footprint.Depth);
        }

        return WriteRegion<void, DStorageCompressionFormat::GDeflate>(data, name.c_str());
    }

    DSFileFormat::Region<DSFileFormat::CpuMetaHeader> WriteCpuMetadata(const std::vector<std::string>& ddsfiles)
    {
        std::vector<uint8> bytes;
        MemoryWriter archive(bytes);

        DSFileFormat::CpuMetaHeader header;

        //placeholder
        auto [fixupHeader] = WriteStruct(archive, &header, &header);

        header.TexturesCount = static_cast<uint32>(texture_descs.size());

        std::vector<DSFileFormat::TexturMetadata> metadatas;
        metadatas.resize(texture_metadatas.size());

        std::vector<Fixup<DSFileFormat::TexturMetadata>> fixups;

        //metadatas placeholder
        for (auto index : std::views::iota(0u, texture_metadatas.size()))
        {
            fixups.emplace_back(std::get<0>(WriteStruct(archive, &metadatas[index], &metadatas[index])));
        }

        for (auto index : std::views::iota(0u, texture_metadatas.size()))
        {
            auto& fixup = fixups[index];
            auto& metadata = metadatas[index];

            metadata.Name = WriteArray(archive, ddsfiles[index]);
            fixup.FixOffset(&metadata, metadata.Name);
            char c = 0;
            archive>>c; // null terminate the name string

            metadata.SingleMipsCount =static_cast<uint32>(texture_metadatas[index].SingleMips.size());
            metadata.SingleMips = WriteArray(archive, texture_metadatas[index].SingleMips);
            fixup.FixOffset(&metadata, metadata.SingleMips);

            //GpuRegion don't need fix
            metadata.RemainingMips = texture_metadatas[index].RemainingMips;

            fixup.Set(archive, metadata);
        }

        header.TexuresDesc = WriteArray(archive, texture_descs);
        fixupHeader.FixOffset(&header, header.TexuresDesc);

        fixupHeader.Set(archive, header);

        return WriteRegion<DSFileFormat::CpuMetaHeader, DStorageCompressionFormat::Zlib>(bytes, "CPU Metadata");
    }

    template<typename T, DStorageCompressionFormat Compression,typename C>
    DSFileFormat::Region<T> WriteRegion(C uncompressedRegion, char const* name)
    {
        size_t uncompressedSize = uncompressedRegion.size();

        C compressedRegion;

        DStorageCompressionFormat compression = Compression;

        {
            compressedRegion = Compress(compression, uncompressedRegion);
            if (compressedRegion.size() >= uncompressedSize)
            {
                compression = DStorageCompressionFormat::None;
                compressedRegion = std::move(uncompressedRegion);
            }
        }

        DSFileFormat::Region<T> r;
        r.Compression = compression;
        r.Data.Offset = static_cast<uint32_t>(out.tellp());
        r.CompressedSize = static_cast<uint32_t>(compressedRegion.size());
        r.UncompressedSize = static_cast<uint32_t>(uncompressedSize);

        out.write(reinterpret_cast<const char*>(compressedRegion.data()), compressedRegion.size());

        auto toString = [](DStorageCompressionFormat c)
            {
                switch (c)
                {
                case DStorageCompressionFormat::None:
                    return "Uncompressed";
                case DStorageCompressionFormat::GDeflate:
                    return "GDeflate";
                case DStorageCompressionFormat::Zlib:
                    return "Zlib";
                default:
                    throw std::runtime_error("Unknown compression format");
                }
            };

        spdlog::info("{}: {} {} {}KB --> {}KB", r.Data.Offset, name, toString(r.Compression), r.UncompressedSize >> 10, r.CompressedSize >> 10);

        return r;
    }

private:
    std::ofstream out;

    struct TextureMetadata
    {
        std::vector<DSFileFormat::GpuRegion> SingleMips;
        DSFileFormat::GpuRegion RemainingMips;
    };

    std::vector<TextureMetadata> texture_metadatas;
    std::vector<DSFileFormat::D3DResourceDesc> texture_descs;

    COMPtr<ID3D12Device> device;

    uint64 stagingBufferSize;
};

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