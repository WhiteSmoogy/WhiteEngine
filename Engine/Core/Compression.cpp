#include "Compression.h"
#include "Compression/lz4hc.h"

namespace WhiteEngine
{
	std::string NAME_LZ4 = "LZ4";

	bool Compression::CompressMemory(const std::string& FormatName, void* CompressedBuffer, int32& CompressedSize, const void* UncompressedBuffer, int32 UncompressedSize, int32 CompressionData)
	{
		bool bCompressSucceeded = false;

		if (_strnicmp(NAME_LZ4.c_str(), FormatName.c_str(), NAME_LZ4.size()) == 0)
		{
			// hardcoded lz4
			CompressedSize = LZ4_compress_HC((const char*)UncompressedBuffer, (char*)CompressedBuffer, UncompressedSize, CompressedSize, LZ4HC_CLEVEL_MAX);
			bCompressSucceeded = CompressedSize > 0;
		}

		return bCompressSucceeded;
	}
}