#pragma once

#include<initializer_list>
#include<WBase/winttype.hpp>

inline white::uint32 MurmurFinalize32(white::uint32 Hash)
{
	Hash ^= Hash >> 16;
	Hash *= 0x85ebca6b;
	Hash ^= Hash >> 13;
	Hash *= 0xc2b2ae35;
	Hash ^= Hash >> 16;
	return Hash;
}

inline white::uint64 MurmurFinalize64(white::uint64 Hash)
{
	Hash ^= Hash >> 33;
	Hash *= 0xff51afd7ed558ccdull;
	Hash ^= Hash >> 33;
	Hash *= 0xc4ceb9fe1a85ec53ull;
	Hash ^= Hash >> 33;
	return Hash;
}

inline white::uint32 Murmur32(std::initializer_list< white::uint32 > InitList)
{
	white::uint32 Hash = 0;
	for (auto Element : InitList)
	{
		Element *= 0xcc9e2d51;
		Element = (Element << 15) | (Element >> (32 - 15));
		Element *= 0x1b873593;

		Hash ^= Element;
		Hash = (Hash << 13) | (Hash >> (32 - 13));
		Hash = Hash * 5 + 0xe6546b64;
	}

	return MurmurFinalize32(Hash);
}