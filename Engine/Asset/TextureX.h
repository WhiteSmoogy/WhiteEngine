/*! \file Engine\Asset\Texture.h
\ingroup Engine
\brief Texture IO/Infomation ...
*/
#ifndef WE_ASSET_TEXTURE_X_H
#define WE_ASSET_TEXTURE_X_H 1

#include "RenderInterface/ITexture.hpp"
#include "Runtime/Core/LFile.h"
#include <WBase/span.hpp>

#include <filesystem>

namespace platform {
	namespace X {
		using path = std::filesystem::path;


		uint64 GetImageInfo(File const & file, Render::TextureType& type,
			uint16& width, uint16& height, uint16& depth, uint8& num_mipmaps, uint8& array_size,
			Render::EFormat& format, uint32& row_pitch, uint32& slice_pitch);

		void GetImageInfo(File const & file, Render::TextureType& type,
			uint16& width, uint16& height, uint16& depth, uint8& num_mipmaps, uint8& array_size,
			Render::EFormat& format,
			std::vector<Render::ElementInitData> & init_data,
			std::vector<uint8>& data_block);

		Render::TexturePtr LoadTexture(path const& texpath, uint32 access);

		void ResizeTexture(void* dst_data, uint32 dst_row_pitch, uint32 dst_slice_pitch,Render::EFormat dst_format,
			uint16 dst_width, uint16 dst_height, uint16 dst_depth,
			void const * src_data, uint32 src_row_pitch, uint32 src_slice_pitch, Render::EFormat src_format,
			uint16 src_width, uint16 src_height, uint16 src_depth,
			bool linear);

		void SaveTexture(path const& path, Render::TextureType type,
			uint32_t width, uint32_t height, uint32_t depth, uint32_t numMipMaps, uint32_t array_size,
			Render::EFormat format, white::span<Render::ElementInitData> init_data);
	}

#ifdef ENGINE_TOOL
	namespace T {
		//���ڴ��ƽ̨����ʹ������ʱ���봦�����ǽ���Դת�����߼�ƥ�����Դ��ʽ
		inline void ImageLE2BE(File const & file) {

		}
	}
#endif
}
#endif