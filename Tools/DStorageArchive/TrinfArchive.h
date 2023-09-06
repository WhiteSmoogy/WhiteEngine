#pragma once

#include "Archive.h"
#include "Asset/TrinfAsset.h"
#include "Asset/MeshX.h"

#include <unordered_map>

constexpr uint32 Mesh_Sign = asset::four_cc_v<'M', 'E', 'S', 'H'>;

class TrinfArchive :public DStorageArchive
{
public:
    using AssetsContainer = std::unordered_map<std::string, std::shared_ptr<asset::MeshAsset>>;

    using Usage = platform::Render::Vertex::Usage;
    using EFormat = platform::Render::IFormat::EFormat;

    TrinfArchive(uint64 stagingBufferMb)
        :DStorageArchive(stagingBufferMb)
    {
    }

    void Archive(const std::string& output, const std::vector<std::string>& assetfiles)
    {
        AssetsContainer meshes;

        uint64 filesize = 0;
        for (auto& file : assetfiles)
        {
            auto asset = platform::X::LoadMeshAsset(file);

            if (asset)
            {
                filesize += fs::file_size(file);
                meshes.emplace(file, asset);
            }
        }

        DSFileFormat::TrinfHeader header{};
        auto [fixHeader] = WriteStruct(archive, &header, &header);

        WriteMeshesBuffer(meshes);
        header.CpuMedadata = WriteCpuMetadata(meshes);

        fixHeader.Set(archive, header);
        {
            std::ofstream out{};
            out.open(output, std::ios::out | std::ios::trunc | std::ios::binary);
            out.write(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        }

        spdlog::info("{} Archived {}->{} MB Ratio {}%", output, filesize >> 20, archive.TotalSize() >> 20, archive.TotalSize() * 100.f / filesize);
    }

    DSFileFormat::Region<DSFileFormat::TrinfGridHeader> WriteCpuMetadata(const AssetsContainer& assets)
    {
        std::vector<uint8> bytes;
        MemoryWriter children_archive(bytes);

        DSFileFormat::TrinfGridHeader gridHeader{};
        gridHeader.StagingBufferSize = stagingBufferSize;

        //placeholder
        auto [fixupHeader] = WriteStruct(children_archive, &gridHeader, &gridHeader);

        gridHeader.Index = WriteRegion<void, DStorageCompressionFormat::GDeflate>(IndexBuffer, "Index");
        gridHeader.Position = WriteRegion<void, DStorageCompressionFormat::GDeflate>(PositionBuffer, "Position");
        gridHeader.Tangent = WriteRegion<void, DStorageCompressionFormat::GDeflate>(TangentBuffer, "Tangent");
        gridHeader.TexCoord = WriteRegion<void, DStorageCompressionFormat::GDeflate>(TexCoordBuffer, "TexCoord");

        std::vector<DSFileFormat::GpuRegion> additionals;
        for (auto& additional : AdditioalVB)
        {
            additionals.emplace_back(WriteRegion<void, DStorageCompressionFormat::GDeflate>(additional,"Additional"));
        }

        gridHeader.AdditioalVB = WriteArray(children_archive, additionals);
        gridHeader.AdditioalVBCount = additionals.size();

        gridHeader.Trinfs.Offset = children_archive.Tell();
        fixupHeader.FixOffset(&gridHeader, gridHeader.Trinfs);

        gridHeader.TrinfsCount = Trinfs.size();

        std::vector<DSFileFormat::TriinfMetadata> metadatas;
        metadatas.resize(Trinfs.size());
        std::vector<Fixup<DSFileFormat::TriinfMetadata>> fixups;

        for (auto& metadata : metadatas)
        {
            fixups.emplace_back(std::get<0>(WriteStruct(children_archive, &metadata, &metadata)));
        }

        for (auto index : std::views::iota(0u, metadatas.size()))
        {
            auto& fixup = fixups[index];
            auto& metadata = metadatas[index];

            metadata.Name = WriteArray(children_archive, Trinfs[index].Name);
            fixup.FixOffset(&metadata, metadata.Name);
            char c = 0;
            children_archive >> c; // null terminate the name string

            metadata.ClusterCompacts = WriteArray(children_archive, Trinfs[index].Compacts);
            metadata.ClusterCount = Trinfs[index].Compacts.size();

            fixup.FixOffset(&metadata, metadata.ClusterCompacts);

            metadata.Clusters = WriteArray(children_archive, Trinfs[index].Clusters);
            fixup.FixOffset(&metadata, metadata.Clusters);

            fixup.Set(children_archive, metadata);
        }

        fixupHeader.Set(children_archive, gridHeader);


        return WriteRegion<DSFileFormat::TrinfGridHeader, DStorageCompressionFormat::Zlib>(bytes, "CPU Metadata");
    }

    void WriteMeshesBuffer(const AssetsContainer& assets)
    {
        int VertexWriteCount = 0;
        for (auto& asset : assets)
        {
            TriinfMetadata metadata;
            metadata.Name = asset.first;

            auto& mesh = asset.second;

            std::vector<uint32> Indexs;
            {
                Indexs.resize(mesh->GetIndexCount());
                auto index_stream = mesh->GetIndexStreams().get();
                auto fill_index = [&](size_t i)
                    {
                        if (mesh->GetIndexFormat() == EF_R16UI)
                        {
                            Indexs[i] = ((uint16*)index_stream)[i];
                        }
                        else
                            Indexs[i] = ((uint32*)index_stream)[i];
                    };
                for (size_t i = 0; i != Indexs.size(); ++i)
                    fill_index(i);
            }

            white::byte empty_element[64] = {};
            auto get_element = [&](Usage usage, uint8 index = 0)
                {
                    auto& elements = mesh->GetVertexElements();
                    for (size_t i = 0; i != elements.size(); ++i)
                    {
                        if (elements[i].usage == usage && elements[i].usage_index == index)
                        {
                            return std::make_pair(mesh->GetVertexStreams()[i].get(), elements[i].format);
                        }
                    }
                    return std::make_pair(empty_element, EF_Unknown);
                };

            auto fill_elem = [](std::pair<white::byte*, EFormat> stream, auto& c,int elemstride = 0)
                {
                    auto stride = NumFormatBytes(stream.second);
                    WAssert(stride <= sizeof(c[0]) - elemstride, "elem out of size");
                    for (size_t i = 0; i != c.size(); ++i)
                    {
                        std::memcpy(reinterpret_cast<white::byte*>(&c[i]) + elemstride, stream.first + stride * i, stride);
                    }
                };

            std::vector<wm::float3> Positions;
            {
                Positions.resize(mesh->GetVertexCount());
                auto pos_stream = get_element(Usage::Position);

                fill_elem(pos_stream, Positions);
            }
            PositionBuffer.insert(PositionBuffer.end(), Positions.begin(), Positions.end());

            {
                std::vector<uint32> Tangents;
                Tangents.resize(mesh->GetVertexCount());
                auto tangent_stream = get_element(Usage::Tangent);

                fill_elem(tangent_stream, Tangents);
                TangentBuffer.insert(TangentBuffer.end(), Tangents.begin(), Tangents.end());
            }

            {
                std::vector<wm::float2> TexCoords;
                TexCoords.resize(mesh->GetVertexCount());
                auto uv_stream = get_element(Usage::TextureCoord, 0);

                fill_elem(uv_stream, TexCoords);
                TexCoordBuffer.insert(TexCoordBuffer.end(), TexCoords.begin(), TexCoords.end());
            }

            //one submesh
            auto desc = mesh->GetSubMeshDesces()[0].LodsDescription[0];

            constexpr int kClusterSize = 64;

            const int triangleCount = desc.IndexNum / 3;
            const int clusterCount = (triangleCount + kClusterSize - 1) / kClusterSize;

            struct Triangle
            {
                wm::float3 vtx[3];
            };

            Triangle triangleCache[kClusterSize];

            for (auto index : std::views::iota(0, clusterCount))
            {
                const uint32 clusterStart = index * kClusterSize;
                const uint32 clusterEnd = std::min<uint32>(clusterStart + kClusterSize, triangleCount);

                const uint32 clusterTriangleCount = clusterEnd - clusterStart;

                // Load all triangles into our local cache
                for (int triangleIndex = clusterStart; triangleIndex < clusterEnd; ++triangleIndex)
                {
                    triangleCache[triangleIndex - clusterStart].vtx[0] = Positions[Indexs[desc.IndexBase + triangleIndex * 3] + desc.VertexBase];
                    triangleCache[triangleIndex - clusterStart].vtx[1] = Positions[Indexs[desc.IndexBase + triangleIndex * 3 + 1] + desc.VertexBase];
                    triangleCache[triangleIndex - clusterStart].vtx[2] = Positions[Indexs[desc.IndexBase + triangleIndex * 3 + 2] + desc.VertexBase];
                }

                wm::float3 aabbMin = { INFINITY, INFINITY, INFINITY };
                wm::float3 aabbMax = -aabbMin;

                for (int triangleIndex = 0; triangleIndex < clusterTriangleCount; ++triangleIndex)
                {
                    const auto& triangle = triangleCache[triangleIndex];
                    for (int j = 0; j < 3; ++j)
                    {
                        aabbMin = wm::min(aabbMin, triangle.vtx[j]);
                        aabbMax = wm::max(aabbMax, triangle.vtx[j]);
                    }
                }

                metadata.Compacts.emplace_back(clusterTriangleCount, static_cast<uint32>(clusterStart + IndexBuffer.size()));
                metadata.Clusters.emplace_back(aabbMin, aabbMax);
            }

            for (auto& index : Indexs)
            {
                index += VertexWriteCount;
            }
            VertexWriteCount += mesh->GetVertexCount();
            IndexBuffer.insert(IndexBuffer.end(), Indexs.begin(), Indexs.end());


            Trinfs.push_back(std::move(metadata));
        }
    }

private:
    std::vector<uint32> IndexBuffer;
    std::vector<white::math::float3> PositionBuffer;
    std::vector<uint32> TangentBuffer;
    std::vector<white::math::float2> TexCoordBuffer;

    std::vector<std::vector<white::math::float4>> AdditioalVB;

    struct TriinfMetadata
    {
        std::string Name;
        std::vector<DSFileFormat::ClusterCompact> Compacts;
        std::vector< DSFileFormat::Cluster> Clusters;
    };

    std::vector<TriinfMetadata> Trinfs;

};