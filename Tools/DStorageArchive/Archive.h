#pragma once

#include <WFramework/Win32/WCLib/COM.h>
#include <WBase/winttype.hpp>
#include <Core/Serialization/MemoryWriter.h>
#include <Core/Serialization/Archive.h>

#include <dstorage.h>
#include <dstorageerr.h>
#include <Asset/DStorageAsset.h>

#include <filesystem>
#include <zlib.h>
#include <ranges>

using namespace platform_ex;
using namespace WhiteEngine;

namespace fs = std::filesystem;

extern COMPtr<IDStorageCompressionCodec> g_gdeflate_codec;

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
            spdlog::error("Failed to compress data using CompressBuffer, hr = {:x}", compressionResult);
            std::abort();
        }

        dest.resize(actualCompressedSize);

        return dest;
    }
}

template<typename T>
void WriteArray(Archive& s, T const* data, size_t count)
{
    s.Serialize(const_cast<T*>(data), sizeof(T) * count);
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

void Patch(Archive& s, int64 pos, void const* data, size_t size)
{
    auto oldPos = s.Tell();
    s.Seek(pos);
    s.Serialize(const_cast<void*>(data), size);
    s.Seek(oldPos);
}

template<typename T>
void Patch(Archive& s, int64 pos, T const& value)
{
    Patch(s, pos, &value, sizeof(value));
}

template<typename T>
void Patch(Archive& s, int64 pos, std::vector<T> const& values)
{
    Patch(s, pos, values.data(), values.size() * sizeof(T));
}

template<typename T>
class Fixup
{
    int64 m_pos;

public:
    Fixup() = default;

    Fixup(int64 fixupPos)
        : m_pos(fixupPos)
    {
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

        field.Offset = static_cast<uint32>(field.Offset - m_pos - offset);
    }
};

template<typename T, typename FIXUP>
Fixup<FIXUP> MakeFixup(int64 startPos, T const* src, FIXUP const* fixup)
{
    auto byteSrc = reinterpret_cast<uint8_t const*>(src);
    auto byteField = reinterpret_cast<uint8_t const*>(fixup);
    std::ptrdiff_t offset = byteField - byteSrc;

    if ((offset + sizeof(*fixup)) > sizeof(*src))
    {
        throw std::runtime_error("Fixup outside of src structure");
    }

    int64 fixupPos = startPos + offset;

    return Fixup<FIXUP>(fixupPos);
}


template<typename T, typename... FIXUPS>
std::tuple<Fixup<FIXUPS>...> WriteStruct(Archive& out, T const* src, FIXUPS const*... fixups)
{
    auto startPos = out.Tell();
    out.Serialize(const_cast<T*>(src), sizeof(*src));

    return std::make_tuple(MakeFixup(startPos, src, fixups)...);
}

class DStorageArchive
{
public:
    DStorageArchive(uint64 stagingBufferMb)
        :bytes(), archive(bytes), stagingBufferSize(stagingBufferMb << 20)
    {
        CheckHResult(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.ReleaseAndGetAddress())));
    }

protected:
    template<typename T, DStorageCompressionFormat Compression, typename C>
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
        r.Data.Offset = static_cast<uint32_t>(archive.Tell());
        r.CompressedSize = static_cast<uint32_t>(compressedRegion.size());
        r.UncompressedSize = static_cast<uint32_t>(uncompressedSize);

        archive.Serialize(const_cast<void*>(reinterpret_cast<const void*>(compressedRegion.data())), compressedRegion.size());

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

protected:
    std::vector<white::byte> bytes;
    MemoryWriter archive;

    COMPtr<ID3D12Device> device;

    uint64 stagingBufferSize;
};