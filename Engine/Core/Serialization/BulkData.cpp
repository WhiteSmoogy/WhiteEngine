#include "BulkData.h"

void WhiteEngine::BulkDataBase::SetBulkDataFlags(uint32 BulkDataFlagsToSet)
{
	BulkDataFlags = EBulkDataFlags(BulkDataFlags | BulkDataFlagsToSet);
}

void* WhiteEngine::FUntypedBulkData::Lock(uint32 LockFlags)
{
	return nullptr;
}

void* WhiteEngine::FUntypedBulkData::Realloc(int64 SizeInBytes)
{
	return nullptr;
}

void WhiteEngine::FUntypedBulkData::Unlock() const
{
}

void WhiteEngine::FUntypedBulkData::SetBulkDataFlags(uint32 BulkDataFlagsToSet)
{
}
