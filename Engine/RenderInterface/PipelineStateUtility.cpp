#include "PipelineStateUtility.h"
#include "IContext.h"
#include <WBase/smart_ptr.hpp>
#include "Core/Hash/CityHash.h"
#include <shared_mutex>
using namespace platform::Render;

namespace platform::Render::PipelineStateCache
{
	class FGraphicsPipelineState
	{
	public:
		std::shared_ptr<GraphicsPipelineState> IPipeline;
	};

	GraphicsPipelineState* ExecuteSetGraphicsPipelineState(FGraphicsPipelineState* PipelineState)
	{
		return PipelineState->IPipeline.get();
	}

	class FComputePipelineState
	{
	public:
		std::shared_ptr<ComputePipelineState> IPipeline;
	};

	ComputePipelineState* ExecuteSetComputePipelineState(FComputePipelineState* PipelineState)
	{
		return PipelineState->IPipeline.get();
	}
}

void platform::Render::SetGraphicsPipelineState(CommandList& cmdlist, const GraphicsPipelineStateInitializer& initializer)
{
	auto PipelineState = PipelineStateCache::GetAndOrCreateGraphicsPipelineState(cmdlist, initializer);

	auto IPipeline = PipelineStateCache::ExecuteSetGraphicsPipelineState(PipelineState);

	cmdlist.SetGraphicsPipelineState(IPipeline);
}

void platform::Render::SetComputePipelineState(ComputeCommandList& cmdlist, const ComputeHWShader* Shader)
{
	auto PipelineState = PipelineStateCache::GetAndOrCreateComputePipelineState(cmdlist, Shader);

	auto IPipeline = PipelineStateCache::ExecuteSetComputePipelineState(PipelineState);

	cmdlist.SetComputePipelineState(IPipeline);
}

struct PipelineStateCacheKeyFuncs
{
	typedef GraphicsPipelineStateInitializer KeyInitType;

	bool operator()(const KeyInitType& A, const KeyInitType& B) const
	{
		if (A.ShaderPass.VertexDeclaration != B.ShaderPass.VertexDeclaration ||
			A.ShaderPass.VertexShader != B.ShaderPass.VertexShader ||
			A.ShaderPass.PixelShader != B.ShaderPass.PixelShader ||
			A.ShaderPass.GeometryShader != B.ShaderPass.GeometryShader ||
			A.ShaderPass.DomainShader != B.ShaderPass.DomainShader ||
			A.ShaderPass.HullShader != B.ShaderPass.HullShader ||
			A.RasterizerState != B.RasterizerState ||
			A.DepthStencilState != B.DepthStencilState ||
			A.BlendState != B.BlendState ||
			A.Primitive != B.Primitive ||
			A.RenderTargetsEnabled != B.RenderTargetsEnabled ||
			A.RenderTargetFormats != B.RenderTargetFormats ||
			A.DepthStencilTargetFormat != B.DepthStencilTargetFormat ||
			A.NumSamples != B.NumSamples
			)
		{
			return false;
		}
		return true;
	}

	std::size_t operator()(const ShaderPassInput& ShaderPass) const
	{
		auto offset = reinterpret_cast<const char*>(&ShaderPass.VertexShader) - reinterpret_cast<const char*>(&ShaderPass);

		auto TailHash = CityHash32(reinterpret_cast<const char*>(&ShaderPass.VertexShader), static_cast<uint32>(sizeof(ShaderPassInput) - offset));

		auto HeadHash = CityHash32(reinterpret_cast<const char*>(ShaderPass.VertexDeclaration.data()), static_cast<uint32>(ShaderPass.VertexDeclaration.size() * sizeof(VertexElement)));

		return ((size_t)HeadHash << 32) | TailHash;
	}

	std::size_t operator()(const KeyInitType& Key) const
	{
		auto offset = reinterpret_cast<const char*>(&Key.RasterizerState) - reinterpret_cast<const char*>(&Key);

		auto TailHash = CityHash64(reinterpret_cast<const char*>(&Key.RasterizerState),static_cast<uint32>(sizeof(KeyInitType) - offset));

		auto HeadHash = operator()(Key.ShaderPass);
		white::hash_combine(HeadHash, TailHash);

		return HeadHash;
	}
};

//TODO: local thread cache
template<class TMyKey, class TMyValue>
class TSharedPipelineStateCache
{
public:
	bool Find(const TMyKey& InKey, TMyValue& OutResult)
	{
		auto itr = CurrentMap.find(InKey);
		if (itr != CurrentMap.end())
		{
			OutResult = itr->second;
			return true;
		}
		return false;
	}

	bool Add(const TMyKey& InKey, TMyValue& OutResult)
	{
		return CurrentMap.emplace(InKey, OutResult).second;
	}
private:
	std::unordered_map< TMyKey, TMyValue, PipelineStateCacheKeyFuncs, PipelineStateCacheKeyFuncs> CurrentMap;
};

using FGraphicsPipelineCache = TSharedPipelineStateCache< GraphicsPipelineStateInitializer, PipelineStateCache::FGraphicsPipelineState*>;

FGraphicsPipelineCache GGraphicsPipelineCache;

PipelineStateCache::FGraphicsPipelineState* PipelineStateCache::GetAndOrCreateGraphicsPipelineState(CommandList& cmdlist, const GraphicsPipelineStateInitializer& OriginalInitializer)
{
	const auto Initializer = &OriginalInitializer;

	FGraphicsPipelineState* OutCachedState = nullptr;

	bool bWasFound = GGraphicsPipelineCache.Find(*Initializer, OutCachedState);

	if (bWasFound == false)
	{
		// create new graphics state
		OutCachedState = new FGraphicsPipelineState();

		OutCachedState->IPipeline = white::share_raw(Context::Instance().GetDevice().CreateGraphicsPipelineState(*Initializer));

		GGraphicsPipelineCache.Add(*Initializer, OutCachedState);
	}

	return OutCachedState;
}

template<class KeyType, class ValueType>
class DiscardableKeyValueCache
{
public:

	/* Flags used when calling Find() */
	struct LockFlags
	{
		enum Flags
		{
			ReadLock = (1 << 0),
			WriteLock = (1 << 1),
			WriteLockOnAddFail = (1 << 2),
		};
	};

	typedef std::unordered_map<KeyType, ValueType> TypedMap;

	DiscardableKeyValueCache() :
		CurrentMap(&Map1)
		, BackfillMap(&Map2) {}

	/* Access to the internal locking object */
	std::shared_mutex& RWLock() { return LockObject; }

	/* Reference to the current map */
	TypedMap& Current() { return *CurrentMap; }

	/* Reference to the current map */
	TypedMap& Backfill() { return *BackfillMap; }

	/* Returns the total number of items in the cache*/
	int32 Num()
	{
		uint32 LockFlags = ApplyLock(0, LockFlags::ReadLock);

		int32 Count = Map1.Num() + Map2.Num();

		Unlock(LockFlags);

		return Count;
	}

	/**
	*  Returns true and sets OutType to the value with the associated key if it exists.
	*/
	bool  Find(const KeyType& Key, ValueType& OutType)
	{
		uint32 LockFlags = ApplyLock(0, LockFlags::ReadLock);

		bool Found = InternalFindWhileLocked(Key, OutType, LockFlags, LockFlags);

		Unlock(LockFlags);
		return Found;
	}

	/**
	 *  Externally-lock-aware Find function.
	 *
	 *	InFlags represents the currently locked state of the object, OutFlags the state after the
	 *	find operation has completed. Caller should be sure to unlock this object with OutFlags
	 */
	bool Find(const KeyType& Key, ValueType& OutType, uint32 InCurrentLockFlags, uint32& OutLockFlags)
	{
		return InternalFindWhileLocked(Key, OutType, InCurrentLockFlags, OutLockFlags);
	}

	/**
	* Add an entry to the current map. Can fail if another thread has inserted
	* a matching object, in which case another call to Find() should succeed
	*/
	bool Add(const KeyType& Key, const ValueType& Value)
	{
		uint32 LockFlags = ApplyLock(0, LockFlags::WriteLock);
		bool Success = Add(Key, Value, LockFlags);
		Unlock(LockFlags);
		return Success;
	}

	/**
	* Add an entry to the current map. Can fail if another thread has inserted
	* a matching object, in which case another call to Find() should succeed
	*/
	bool Add(const KeyType& Key, const ValueType& Value, const uint32 LockFlags)
	{
		bool Success = true;

		WAssert((LockFlags & LockFlags::WriteLock) != 0,"Cache is not locked for write during Add!");

		// key is already here! likely another thread may have filled the cache. calling code should handle this
		// or request that a write lock be left after a Find() fails.
		if (CurrentMap->contains(Key) == false)
		{
			CurrentMap->emplace(Key, Value);
		}
		else
		{
			Success = false;
		}

		return Success;
	}

	/*
		Discard all items left in the backfill and swap the current & backfill pointers
	*/
	int32 Discard()
	{
		uint32 LockFlags = ApplyLock(0, LockFlags::WriteLock);

		int32 Discarded = Discard(LockFlags, LockFlags, [](ValueType& Type) {});

		Unlock(LockFlags);

		return Discarded;
	}

	template<typename DeleteFunc>
	int32 Discard(DeleteFunc Func)
	{
		uint32 LockFlags = ApplyLock(0, LockFlags::WriteLock);

		int32 Discarded = Discard(LockFlags, LockFlags, Func);

		Unlock(LockFlags);

		return Discarded;
	}

	/**
	 * Discard all items in the backfill and swap the current & backfill pointers
	 */
	template<typename DeleteFunc>
	int32 Discard(uint32 InCurrentLockFlags, uint32& OutNewLockFlags, DeleteFunc Func)
	{
		if ((InCurrentLockFlags & LockFlags::WriteLock) == 0)
		{
			InCurrentLockFlags = ApplyLock(InCurrentLockFlags, LockFlags::WriteLock);
		}

		for (auto& KV : *BackfillMap)
		{
			Func(KV.Value);
		}

		int32 Discarded = BackfillMap->size();

		// free anything still in the backfill map
		BackfillMap->clear();

		if (CurrentMap == &Map1)
		{
			CurrentMap = &Map2;
			BackfillMap = &Map1;
		}
		else
		{
			CurrentMap = &Map1;
			BackfillMap = &Map2;
		}

		OutNewLockFlags = InCurrentLockFlags;
		return Discarded;
	}

public:
	uint32 ApplyLock(uint32 CurrentFlags, uint32 NewFlags)
	{
		bool IsLockedForRead = (CurrentFlags & LockFlags::ReadLock) != 0;
		bool IsLockedForWrite = (CurrentFlags & LockFlags::WriteLock) != 0;

		bool WantLockForRead = (NewFlags & LockFlags::ReadLock) != 0;
		bool WantLockForWrite = (NewFlags & LockFlags::WriteLock) != 0;


		// if already locked for write, nothing to do
		if (IsLockedForWrite && (WantLockForWrite || WantLockForRead))
		{
			return LockFlags::WriteLock;
		}

		// if locked for reads and that's all that's needed, d
		if (IsLockedForRead && WantLockForRead && !WantLockForWrite)
		{
			return LockFlags::ReadLock;
		}

		Unlock(CurrentFlags);

		// chance they asked for both Read/Write, so check this first
		if (WantLockForWrite)
		{
			LockObject.lock();
		}
		else if (WantLockForRead)
		{
			LockObject.lock_shared();
		}

		return NewFlags;
	}

	void Unlock(uint32 Flags)
	{
		bool LockedForRead = (Flags & LockFlags::ReadLock) != 0;
		bool LockedForWrite = (Flags & LockFlags::WriteLock) != 0;

		if (LockedForWrite)
		{
			LockObject.unlock();
		}
		else if (LockedForRead)
		{
			LockObject.unlock_shared();
		}
	}

protected:

	/**
	*	Checks for the entry in our current map, and if not found the backfill. If the entry is in the backfill it is moved
	*	to the current map so it will not be discarded when DiscardUnusedEntries is called
	*/
	bool  InternalFindWhileLocked(const KeyType& Key, ValueType& OutType, uint32 InCurrentLockFlags, uint32& OutFlags)
	{
		// by default we'll do exactly what was asked...

		bool LockedForWrite = (InCurrentLockFlags & LockFlags::WriteLock) != 0;
		bool LeaveWriteLockOnFailure = (InCurrentLockFlags & LockFlags::WriteLockOnAddFail) != 0;

		uint32 CurrentFlags = InCurrentLockFlags;

		WAssert((CurrentFlags & LockFlags::ReadLock) != 0
			|| (CurrentFlags & LockFlags::WriteLock) != 0,
			"Cache is not locked for read or write during Find!");

		// Do we have this?
		auto Found = CurrentMap->find(Key);

		// If not check the backfill,  if it's there remove it add it to our map. 
		if (Found == CurrentMap->end())
		{
			auto BackfillFound = BackfillMap->find(Key);

			// we either need to lock to adjust our cache, or lock because the user wants to...
			bool NeedWriteLock = (BackfillFound != BackfillMap->end() || LeaveWriteLockOnFailure);

			if (NeedWriteLock)
			{
				// lock the buffer (nop if we were already locked!)
				CurrentFlags = ApplyLock(CurrentFlags, CurrentFlags | LockFlags::WriteLock);

				// check again, there's a chance these may have changed filled between the unlock/lock
				// above..
				Found = CurrentMap->find(Key);
				if (Found == CurrentMap->end())
				{
					BackfillFound = BackfillMap->find(Key);
				}
			}

			// If we found a backfill, move it to the primary
			if (Found == CurrentMap->end() && BackfillFound != BackfillMap->end())
			{
				// if shared refs, add/remove order is important
				Found = CurrentMap->emplace(Key, BackfillFound->second).first;
				BackfillMap->erase(Key);
			}
		}

		if (Found != CurrentMap->end())
		{
			OutType = Found->second;
		}

		OutFlags = CurrentFlags;
		return Found != CurrentMap->end();
	}


	std::shared_mutex			LockObject;
	TypedMap* CurrentMap;
	TypedMap* BackfillMap;
	TypedMap		Map1;
	TypedMap		Map2;
};

using FComputePipelineCache = DiscardableKeyValueCache<const ComputeHWShader*, PipelineStateCache::FComputePipelineState*>;
FComputePipelineCache GComputePipelineCache;

PipelineStateCache::FComputePipelineState* PipelineStateCache::GetAndOrCreateComputePipelineState(ComputeCommandList& cmdlist, const ComputeHWShader* Shader)
{
	FComputePipelineState* OutCachedState = nullptr;

	uint32 LockFlags = GComputePipelineCache.ApplyLock(0, FComputePipelineCache::LockFlags::ReadLock);

	bool WasFound = GComputePipelineCache.Find(Shader, OutCachedState, LockFlags | FComputePipelineCache::LockFlags::WriteLockOnAddFail, LockFlags);

	if (!WasFound)
	{
		OutCachedState = new FComputePipelineState();

		OutCachedState->IPipeline = white::share_raw(Context::Instance().GetDevice().CreateComputePipelineState(Shader));

		GComputePipelineCache.Add(Shader, OutCachedState, LockFlags);
	}

	GComputePipelineCache.Unlock(LockFlags);

	return OutCachedState;
}



