#pragma once

#include "CoreTypes.h"

namespace WhiteEngine
{
	class PageAllocator
	{
	public:
		static constexpr int32 PageSize = 64 * 1024;
		static constexpr int32 SmallPageSize = 1024-16;

		static PageAllocator& Get();

		void* AllocSmall();
		void* Alloc();
		void Free(void* mem);
		void FreeSmall(void* mem);
	};

	class MemStackBase
	{
	public:
		MemStackBase(int32 InMinMarksToAlloc = 1)
			: Top(nullptr)
			, End(nullptr)
			, TopChunk(nullptr)
			, TopMark(nullptr)
			, NumMarks(0)
			, MinMarksToAlloc(InMinMarksToAlloc)
			, bShouldEnforceAllocMarks(false)
		{

		}

		~MemStackBase()
		{
			wconstraint((!NumMarks));
			FreeChunks(nullptr);
		}

		uint8* PushBytes(int32 AllocSize, int32 Alignment)
		{
			return (uint8*)Alloc(AllocSize, std::max(AllocSize >= 16 ? (int32)16 : (int32)8, Alignment));
		}

		void* Alloc(int32 AllocSize, int32 Alignment)
		{
			// Debug checks.
			wconstraint(AllocSize >= 0);
			wconstraint((Alignment & (Alignment - 1)) == 0);
			wconstraint(Top <= End);
			wconstraint(NumMarks >= MinMarksToAlloc);
			wconstraint(!bShouldEnforceAllocMarks || NumMarks > 0);

			// Try to get memory from the current chunk.
			uint8* Result = white::Align(Top, Alignment);
			uint8* NewTop = Result + AllocSize;

			// Make sure we didn't overflow.
			if (NewTop <= End)
			{
				Top = NewTop;
			}
			else
			{
				// We'd pass the end of the current chunk, so allocate a new one.
				AllocateNewChunk(AllocSize + Alignment);
				Result = white::Align(Top, Alignment);
				NewTop = Result + AllocSize;
				Top = NewTop;
			}
			return Result;
		}

		/** return true if this stack is empty. */
		bool IsEmpty() const
		{
			return TopChunk == nullptr;
		}

		void Flush()
		{
			wconstraint(!NumMarks && !MinMarksToAlloc);
			FreeChunks(nullptr);
		}
		int32 GetNumMarks()
		{
			return NumMarks;
		}
		/** @return the number of bytes allocated for this FMemStack that are currently in use. */
		int32 GetByteCount() const;

		// Returns true if the pointer was allocated using this allocator
		bool ContainsPointer(const void* Pointer) const;

		// Types.
		struct FTaggedMemory
		{
			FTaggedMemory* Next;
			int32 DataSize;

			uint8* Data() const
			{
				return ((uint8*)this) + sizeof(FTaggedMemory);
			}
		};

	private:

		/**
		 * Allocate a new chunk of memory of at least MinSize size,
		 * updates the memory stack's Chunks table and ActiveChunks counter.
		 */
		void AllocateNewChunk(int32 MinSize);

		/** Frees the chunks above the specified chunk on the stack. */
		void FreeChunks(FTaggedMemory* NewTopChunk);

		// Variables.
		uint8* Top;				// Top of current chunk (Top<=End).
		uint8* End;				// End of current chunk.
		FTaggedMemory* TopChunk;			// Only chunks 0..ActiveChunks-1 are valid.

		/** The top mark on the stack. */
		class FMemMark* TopMark;

		/** The number of marks on this stack. */
		int32 NumMarks;

		/** Used for a constraint. Most stacks require a mark to allocate. Command lists don't because they never mark, only flush*/
		int32 MinMarksToAlloc;

	protected:
		bool bShouldEnforceAllocMarks;
	};
}

inline void* operator new(size_t Size, WhiteEngine::MemStackBase& Mem, int32 Count = 1, int32 Align = 0)
{
	// Get uninitialized memory.
	const size_t SizeInBytes = Size * Count;
	wconstraint(SizeInBytes <= (size_t)std::numeric_limits<int32>::max());
	return Mem.PushBytes((int32)SizeInBytes, Align);
}

inline void* operator new[](size_t Size, WhiteEngine::MemStackBase& Mem, int32 Count = 1, int32 Align = 0)
{
	// Get uninitialized memory.
	const size_t SizeInBytes = Size * Count;
	wconstraint(SizeInBytes <= (size_t)std::numeric_limits<int32>::max());
	return Mem.PushBytes((int32)SizeInBytes, Align);
}