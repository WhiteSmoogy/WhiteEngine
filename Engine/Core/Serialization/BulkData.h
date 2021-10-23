#pragma once
#include "CoreTypes.h"
namespace WhiteEngine
{
	enum EBulkDataFlags : uint32
	{
		/** Empty flag set. */
		BULKDATA_None = 0,

		/** There is a new default to inline unless you opt out. */
		BULKDATA_Force_NOT_InlinePayload = 1 << 10,
	};

	/**
	 * Enumeration for bulk data lock behavior
	*/
	enum EBulkDataLockFlags
	{
		LOCK_READ_ONLY = 1,
		LOCK_READ_WRITE = 2,
	};

	class BulkDataBase
	{
	public:
		void* Lock(uint32 LockFlags);

		void* Realloc(int64 SizeInBytes);

		void Unlock() const;

		void SetBulkDataFlags(uint32 BulkDataFlagsToSet);

	private:
		EBulkDataFlags BulkDataFlags = BULKDATA_None;
	};
	
	template<typename ElementType>
	class TUntypedBulkData:public BulkDataBase
	{};

	class FUntypedBulkData
	{
	public:
		void* Lock(uint32 LockFlags);

		void* Realloc(int64 SizeInBytes);

		void Unlock() const;

		void SetBulkDataFlags(uint32 BulkDataFlagsToSet);

	private:
		EBulkDataFlags BulkDataFlags = BULKDATA_None;
	};

	class FByteBulkData :public FUntypedBulkData
	{

	};

#if !WE_EDITOR
	using ByteBulkData = TUntypedBulkData<uint8>;
#else
	using ByteBulkData = FByteBulkData;
#endif
}