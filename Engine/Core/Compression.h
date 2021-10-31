#pragma once
#include <WBase/wdef.h>
#include <CoreTypes.h>
#include <string>
namespace WhiteEngine
{
	extern std::string NAME_LZ4;

	namespace Compression
	{
		bool CompressMemory(const std::string& FormatName, void* CompressedBuffer, int32& CompressedSize, const void* UncompressedBuffer, int32 UncompressedSize,int32 CompressionData = 0);

		bool UnCompressMemory(const std::string& FormatName, void* UncompressedBuffer, int32 UncompressedSize, const void* CompressedBuffer, int32 CompressedSize);
	}
}