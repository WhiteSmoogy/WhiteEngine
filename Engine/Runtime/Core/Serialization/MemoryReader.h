#pragma once

#include <WBase/span.hpp>
#include "MemoryArchive.h"

namespace WhiteEngine
{
	/**
 * Archive for reading arbitrary data from the specified array view
 */
	class MemoryReaderView : public MemoryArchive
	{
	public:
		int64 TotalSize() 
		{
			return std::min<int64>((int64)Bytes.size(), LimitSize);
		}

		MemoryReaderView& Serialize(void* Data, uint64 Num) override
		{
			if (Num && !ArIsError)
			{
				// Only serialize if we have the requested amount of data
				if (static_cast<int64>(Offset + Num) <= TotalSize())
				{
					std::memcpy(Data, &Bytes[(int32)Offset], Num);
					Offset += Num;
				}
				else
				{
					ArIsError = true;
				}
			}
			return *this;
		}

		explicit MemoryReaderView(white::span<const uint8> InBytes)
			: Bytes(InBytes)
			, LimitSize(INT64_MAX)
		{
			ArIsLoading = true;
		}

		/** With this method it's possible to attach data behind some serialized data. */
		void SetLimitSize(int64 NewLimitSize)
		{
			LimitSize = NewLimitSize;
		}

	private:
		white::span<const uint8> Bytes;
		int64                LimitSize;
	};
}
