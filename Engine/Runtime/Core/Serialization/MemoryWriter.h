#pragma once
#include "MemoryArchive.h"
#include "spdlog/spdlog.h"

namespace WhiteEngine
{
	/**
	* Archive for storing arbitrary data to the specified memory location
	*/
	class MemoryWriter : public MemoryArchive
	{
	public:
		MemoryWriter(std::vector<white::uint8>& InBytes)
			: MemoryArchive()
			, Bytes(InBytes)
		{
			ArIsSaving = true;
		}

		virtual MemoryWriter& Serialize(void* Data, white::uint64 Num) override
		{
			const int64 NumBytesToAdd = Offset + Num - Bytes.size();
			if (NumBytesToAdd > 0)
			{
				const int64 NewArrayCount = Bytes.size() + NumBytesToAdd;
				if (NewArrayCount >= std::numeric_limits<int32>::max())
				{
					spdlog::error("MemoryWriter does not support data larger than 2GB.");
				}

				Bytes.resize(Bytes.size()+(int32)NumBytesToAdd);
			}

			wconstraint((Offset + Num) <= Bytes.size());

			if (Num)
			{
				std::memcpy(&Bytes[(int32)Offset], Data, Num);
				Offset += Num;
			}

			return *this;
		}

		int64 TotalSize()
		{
			return Bytes.size();
		}
	protected:
		std::vector<uint8>& Bytes;
	};
}