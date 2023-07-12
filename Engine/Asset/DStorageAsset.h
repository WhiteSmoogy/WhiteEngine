#pragma once

#include "RenderInterface/DStorage.h"
#include "Runtime/Core/Coroutine/Task.h"
#include <filesystem>

namespace platform_ex
{
	using path = std::filesystem::path;

	namespace DSFileFormat
	{
		/*
		* On disk, this is an offset relative to the containing
		*/
		template<typename type>
		struct OffsetPtr
		{
			uint32 Offset;

			operator const type* () const
			{
				return reinterpret_cast<type*>(reinterpret_cast<white::byte*>(this) + Offset);
			}

			operator type* ()
			{
				return reinterpret_cast<type*>(reinterpret_cast<white::byte*>(this) + Offset);
			}
		};

		// OffsetPtr with overloaded array access operators.
		template<typename type>
		struct OffsetArray :OffsetPtr<type>
		{
			const type* data() const
			{
				return this->operator const type * ();
			}

			type* data()
			{
				return this->operator type * ();
			}

			type& operator[] (size_t index)
			{
				return data()[index];
			}

			type const& operator[] (size_t index) const
			{
				return data()[index];
			}
		};

		/* 
		* A 'Region' describes a part of the file that can be read with a single
		* DirectStorage request.  Each region can choose its own compression
		* format.
		*/
		template<typename T>
		struct Region
		{
			DStorageCompressionFormat Compression;
			OffsetPtr<T> Data;
			uint32 CompressedSize;
			uint32 UncompressedSize;
		};

		// A Region that's loaded into GPU memory
		using GpuRegion = Region<void>;

		struct TexturMetadata
		{
			OffsetPtr<char> Name;
			OffsetArray<GpuRegion> SingleMips;
			uint32 SingleMipsCount;
			GpuRegion RemainingMips;
		};

		struct D3DResourceDesc
		{
			uint32 Dimension;
			uint64 Alignment;
			uint64 Width;
			uint32 Height;
			uint16 DepthOrArraySize;
			uint16 MipLevels;
			uint32 Format;
			struct {
				uint32 Count;
				uint32 Quality;
			}SampleDesc;
			uint32 Layout;
			uint32 Flags;
		};

		//256MB
		constexpr uint32 kDefaultStagingBufferSize = 256 * 1024 * 1024;

		struct CpuMetaHeader
		{
			uint32 StagingBufferSize;

			OffsetArray<TexturMetadata> Textures;
			uint32 TexturesCount;
			OffsetArray<D3DResourceDesc> TexuresDesc;
		};

		struct Header
		{
			uint32 Signature; //'DSFF'
			white::uint32 Version;

			Region<CpuMetaHeader> CpuMedadata;
		};
	}

	white::coroutine::Task<void> AsyncLoadDStorageAsset(path const& assetpath);
}