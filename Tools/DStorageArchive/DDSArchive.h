#pragma once

#include "Archive.h"
#include "RenderInterface/IFormat.hpp"
#include "RenderInterface/ITexture.hpp"
#include "Runtime/LFile.h"
#include "Asset/TextureX.h"
#include "Asset/DDSX.h"
#include "D3D12/d3d12_dxgi.h"

#include <iostream>

using namespace white::inttype;
using namespace platform_ex;

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

class DDSArchive:public DStorageArchive
{
public:
    DDSArchive(uint64 stagingBufferMb)
        :DStorageArchive(stagingBufferMb)
    {
    }

    void Archive(const std::string& output, const std::vector<std::string>& ddsfiles)
    {
        DSFileFormat::Header header;
        header.Signature = DSFileFormat::DSTexture_Signature;
        header.Version = 0;

        //placeholder
        auto [fixHeader] = WriteStruct(archive, &header, &header);

        WriteTextures(ddsfiles);
        header.CpuMedadata = WriteCpuMetadata(ddsfiles);

        fixHeader.Set(archive, header);

        {
            std::ofstream out{};
            out.open(output, std::ios::out | std::ios::trunc | std::ios::binary);
            out.write(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        }

        uint64 filesize = 0;
        for (auto& file : ddsfiles)
        {
            filesize += fs::file_size(file);
        }

        spdlog::info("{} Archived {}->{} MB Ratio {}%", output, filesize >> 20, archive.TotalSize() >> 20, archive.TotalSize()*100.f / filesize);
    }

    ~DDSArchive()
    {
    }
private:
    void  WriteTextures(const std::vector<std::string>& ddsfiles)
    {
        for (auto& ddsfile : ddsfiles)
        {
            fs::path path{ ddsfile };

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
                    spdlog::error("Try add --stagingsize={} to the command-line", (totalBytes + 1024 * 1024 - 1) / 1024 / 1024);
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
        MemoryWriter children_archive(bytes);

        DSFileFormat::CpuMetaHeader header;

        //placeholder
        auto [fixupHeader] = WriteStruct(children_archive, &header, &header);

        header.TexturesCount = static_cast<uint32>(texture_descs.size());
        header.StagingBufferSize = stagingBufferSize;

        std::vector<DSFileFormat::TexturMetadata> metadatas;
        metadatas.resize(texture_metadatas.size());

        std::vector<Fixup<DSFileFormat::TexturMetadata>> fixups;

        header.Textures.Offset = children_archive.Tell();
        //metadatas placeholder
        for (auto index : std::views::iota(0u, texture_metadatas.size()))
        {
            fixups.emplace_back(std::get<0>(WriteStruct(children_archive, &metadatas[index], &metadatas[index])));
        }

        for (auto index : std::views::iota(0u, texture_metadatas.size()))
        {
            auto& fixup = fixups[index];
            auto& metadata = metadatas[index];

            metadata.Name = WriteArray(children_archive, ddsfiles[index]);
            fixup.FixOffset(&metadata, metadata.Name);
            char c = 0;
            children_archive >> c; // null terminate the name string

            metadata.SingleMipsCount = static_cast<uint32>(texture_metadatas[index].SingleMips.size());
            metadata.SingleMips = WriteArray(children_archive, texture_metadatas[index].SingleMips);
            fixup.FixOffset(&metadata, metadata.SingleMips);

            //GpuRegion don't need fix
            metadata.RemainingMips = texture_metadatas[index].RemainingMips;

            fixup.Set(children_archive, metadata);
        }

        header.TexuresDesc = WriteArray(children_archive, texture_descs);
        fixupHeader.FixOffset(&header, header.TexuresDesc);

        fixupHeader.Set(children_archive, header);

        return WriteRegion<DSFileFormat::CpuMetaHeader, DStorageCompressionFormat::Zlib>(bytes, "CPU Metadata");
    }

private:
    struct TextureMetadata
    {
        std::vector<DSFileFormat::GpuRegion> SingleMips;
        DSFileFormat::GpuRegion RemainingMips;
    };

    std::vector<TextureMetadata> texture_metadatas;
    std::vector<DSFileFormat::D3DResourceDesc> texture_descs;
};
