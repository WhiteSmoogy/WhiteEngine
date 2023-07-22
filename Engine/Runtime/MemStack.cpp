#include "MemStack.h"

using namespace white;
using namespace WhiteEngine;

PageAllocator& WhiteEngine::PageAllocator::Get()
{
	static PageAllocator Instance;

	return Instance;
}

//TODO: https://github.com/microsoft/mimalloc
void* WhiteEngine::PageAllocator::AllocSmall()
{
	void* Result = ::operator new(SmallPageSize);
	return Result;
}

void* WhiteEngine::PageAllocator::Alloc()
{
	void* Result = ::operator new(PageSize);
	return Result;
}

void WhiteEngine::PageAllocator::FreeSmall(void* mem)
{
	::operator delete(mem);
}

void WhiteEngine::PageAllocator::Free(void* mem)
{
	::operator delete(mem);
}

int32 WhiteEngine::MemStackBase::GetByteCount() const
{
	int32 Count = 0;
	for (FTaggedMemory* Chunk = TopChunk; Chunk; Chunk = Chunk->Next)
	{
		if (Chunk != TopChunk)
		{
			Count += Chunk->DataSize;
		}
		else
		{
			Count += static_cast<int32>(Top - Chunk->Data());
		}
	}
	return Count;
}

void MemStackBase::AllocateNewChunk(int32 MinSize)
{
	FTaggedMemory* Chunk = nullptr;
	// Create new chunk.
	int32 TotalSize = MinSize + (int32)sizeof(FTaggedMemory);
	uint32 AllocSize;
	if (TopChunk || TotalSize > PageAllocator::SmallPageSize)
	{
		AllocSize = AlignArbitrary<int32>(TotalSize, PageAllocator::PageSize);
		if (AllocSize == PageAllocator::PageSize)
		{
			Chunk = (FTaggedMemory*)PageAllocator::Get().Alloc();
		}
		else
		{
			Chunk = (FTaggedMemory*)::operator new(AllocSize);
		}
		wconstraint(AllocSize != PageAllocator::SmallPageSize);
	}
	else
	{
		AllocSize = PageAllocator::SmallPageSize;
		Chunk = (FTaggedMemory*)PageAllocator::Get().AllocSmall();
	}
	Chunk->DataSize = AllocSize - sizeof(FTaggedMemory);

	Chunk->Next = TopChunk;
	TopChunk = Chunk;
	Top = Chunk->Data();
	End = Top + Chunk->DataSize;
}

void MemStackBase::FreeChunks(FTaggedMemory* NewTopChunk)
{
	while (TopChunk != NewTopChunk)
	{
		FTaggedMemory* RemoveChunk = TopChunk;
		TopChunk = TopChunk->Next;
		if (RemoveChunk->DataSize + sizeof(FTaggedMemory) == PageAllocator::PageSize)
		{
			PageAllocator::Get().Free(RemoveChunk);
		}
		else if (RemoveChunk->DataSize + sizeof(FTaggedMemory) == PageAllocator::SmallPageSize)
		{
			PageAllocator::Get().FreeSmall(RemoveChunk);
		}
		else
		{
			::operator delete(RemoveChunk);
		}
	}
	Top = nullptr;
	End = nullptr;
	if (TopChunk)
	{
		Top = TopChunk->Data();
		End = Top + TopChunk->DataSize;
	}
}

bool MemStackBase::ContainsPointer(const void* Pointer) const
{
	const uint8* Ptr = (const uint8*)Pointer;
	for (const FTaggedMemory* Chunk = TopChunk; Chunk; Chunk = Chunk->Next)
	{
		if (Ptr >= Chunk->Data() && Ptr < Chunk->Data() + Chunk->DataSize)
		{
			return true;
		}
	}

	return false;
}
