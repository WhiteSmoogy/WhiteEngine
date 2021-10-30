#pragma once

#include <WBase/winttype.hpp>
#include <string>
#include <string_view>

namespace Digest {
	using namespace white::inttype;

	[[nodiscard]]
	inline std::string BytesToHex(const uint8* In, int32 Count);

	int32 HexToBytes(std::string_view Hex, uint8* OutBytes);

	class SHAHash
	{
	public:
		alignas(uint32) uint8 Hash[20];

		friend bool operator==(const SHAHash& X, const SHAHash& Y)
		{
			return std::memcmp(&X.Hash, &Y.Hash, sizeof(X.Hash)) == 0;
		}
	};

	/*-----------------------------------------------------------------------------
	SHA-1 functions.
	-----------------------------------------------------------------------------*/
	
	/*
	 *	NOTE:
	 *	100% free public domain implementation of the SHA-1 algorithm
	 *	by Dominik Reichl <dominik.reichl@t-online.de>
	 *	Web: http://www.dominik-reichl.de/
	 */

	typedef union
	{
		uint8  c[64];
		uint32 l[16];
	} SHA1_WORKSPACE_BLOCK;

	class SHA1
	{
	public:

		enum { DigestSize = 20 };
		// Constructor and Destructor
		SHA1();
		~SHA1();

		uint32 m_state[5];
		uint32 m_count[2];
		uint32 __reserved1[1];
		uint8  m_buffer[64];
		uint8  m_digest[20];
		uint32 __reserved2[3];

		void Reset();

		// Update the hash value
		void Update(const uint8* data, uint64 len);

		// Finalize hash and report
		void Final();


		// Report functions: as pre-formatted and raw data
		void GetHash(uint8* puDest);

		/**
		 * Calculate the hash on a single block and return it
		 *
		 * @param Data Input data to hash
		 * @param DataSize Size of the Data block
		 * @param OutHash Resulting hash value (20 byte buffer)
		 */
		static void HashBuffer(const void* Data, uint64 DataSize, uint8* OutHash);

		/**
		 * Calculate the hash on a single block and return it
		 *
		 * @param Data Input data to hash
		 * @param DataSize Size of the Data block
		 * @return Resulting digest
		 */
		static SHAHash HashBuffer(const void* Data, uint64 DataSize)
		{
			SHAHash Hash;
			HashBuffer(Data, DataSize, Hash.Hash);
			return Hash;
		}
	private:
		// Private SHA-1 transformation
		void Transform(uint32* state, const uint8* buffer);

		// Member variables
		uint8 m_workspace[64];
		SHA1_WORKSPACE_BLOCK* m_block; // SHA1 pointer to the byte array above
	};

}