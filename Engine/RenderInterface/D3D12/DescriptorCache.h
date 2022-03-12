#pragma once

#include <WBase/wdef.h>
#include "Runtime/Core/Hash/CityHash.h"

#include "../ShaderCore.h"

#include "d3d12_dxgi.h"
#include "Common.h"
#include "D3DCommandList.h"
#include <unordered_set>
#include <mutex>
#include <queue>

namespace platform_ex::Windows::D3D12 {
	class CommandContext;
	class DescriptorCache;
	class RenderTargetView;
	class DepthStencilView;

	struct VertexBufferCache;
	struct UnorderedAccessViewCache;
	struct ShaderResourceViewCache;
	struct SamplerStateCache;
	struct ConstantBufferCache;

	class RootSignature;
	class ResourceHolder;

	class SamplerState;

	using platform::Render::ShaderType;

	// Like a std::unordered_map<KeyType, ValueType>
	// Faster lookup performance, but possibly has false negatives
	template<typename KeyType, typename ValueType>
	class ConservativeMap
	{
	public:
		ConservativeMap(uint32 Size)
		{
			Table.resize(Size);

			Reset();
		}

		void Add(const KeyType& Key, const ValueType& Value)
		{
			uint32 Index = GetIndex(Key);

			Entry& Pair = Table[Index];

			Pair.Valid = true;
			Pair.Key = Key;
			Pair.Value = Value;
		}

		ValueType* Find(const KeyType& Key)
		{
			uint32 Index = GetIndex(Key);

			Entry& Pair = Table[Index];

			if (Pair.Valid &&
				(Pair.Key == Key))
			{
				return &Pair.Value;
			}
			else
			{
				return nullptr;
			}
		}

		void Reset()
		{
			for (int32 i = 0; i < Table.size(); i++)
			{
				Table[i].Valid = false;
			}
		}
	private:
		uint32 GetIndex(const KeyType& Key)
		{
			uint32 Hash = GetTypeHash(Key);

			return Hash % static_cast<uint32>(Table.size());
		}

		struct Entry
		{
			bool Valid = false;
			KeyType Key;
			ValueType Value;
		};

		std::vector<Entry> Table;
	};

	struct SamplerArrayDesc
	{
		uint32 Count;
		uint16 SamplerID[16];

		bool operator==(const SamplerArrayDesc& rhs) const
		{
			if (Count != rhs.Count)
			{
				return false;
			}
			else
			{
				return 0 == std::memcmp(SamplerID, rhs.SamplerID, sizeof(SamplerID[0]) * Count);
			}
		}
	};

	uint32 GetTypeHash(const SamplerArrayDesc& Key);

	using SamplerMap = ConservativeMap<SamplerArrayDesc, D3D12_GPU_DESCRIPTOR_HANDLE>;

	template< uint32 CPUTableSize>
	struct UniqueDescriptorTable
	{
		UniqueDescriptorTable() : GPUHandle({}) {};
		UniqueDescriptorTable(SamplerArrayDesc KeyIn, CD3DX12_CPU_DESCRIPTOR_HANDLE* Table) : GPUHandle({})
		{
			std::memcpy(&Key, &KeyIn, sizeof(Key));//Memcpy to avoid alignement issues
			std::memcpy(CPUTable, Table, Key.Count * sizeof(CD3DX12_CPU_DESCRIPTOR_HANDLE));
		}

		static uint64 GetTypeHash(const UniqueDescriptorTable& Table)
		{
			return CityHash64((char*)Table.Key.SamplerID, Table.Key.Count * sizeof(Table.Key.SamplerID[0]));
		}

		SamplerArrayDesc Key;
		CD3DX12_CPU_DESCRIPTOR_HANDLE CPUTable[MAX_SAMPLERS];

		// This will point to the table start in the global heap
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;

		bool operator==(const UniqueDescriptorTable& rhs) const
		{
			return Key == rhs.Key;
		}
	};

	template<typename UniqueSamplerTable>
	struct UniqueDescriptorTableHasher
	{
		size_t operator()(const UniqueSamplerTable& key) const noexcept {
			return UniqueSamplerTable::GetTypeHash(key);
		}
	};

	using UniqueSamplerTable = UniqueDescriptorTable<MAX_SAMPLERS>;

	using SamplerSet = std::unordered_set<UniqueSamplerTable, UniqueDescriptorTableHasher<UniqueSamplerTable>>;

	class OfflineDescriptorManager : public SingleNodeGPUObject
	{
	public: // Types
		typedef D3D12_CPU_DESCRIPTOR_HANDLE HeapOffset;
		typedef decltype(HeapOffset::ptr) HeapOffsetRaw;
		typedef uint32 HeapIndex;

	private: // Types
		struct SFreeRange { HeapOffsetRaw Start; HeapOffsetRaw End; };
		struct SHeapEntry
		{
			COMPtr<ID3D12DescriptorHeap> m_Heap;
			std::list<SFreeRange> m_FreeList;

			SHeapEntry() { }
		};
		typedef std::vector<SHeapEntry> THeapMap;

		static D3D12_DESCRIPTOR_HEAP_DESC CreateDescriptor(GPUMaskType Node, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptorsPerHeap)
		{
			D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
			Desc.Type = Type;
			Desc.NumDescriptors = NumDescriptorsPerHeap;
			Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;// None as this heap is offline
			Desc.NodeMask = Node.GetNative();

			return Desc;
		}

	public: // Methods
		OfflineDescriptorManager(GPUMaskType Node, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptorsPerHeap)
			: SingleNodeGPUObject(Node)
			, m_Desc(CreateDescriptor(Node, Type, NumDescriptorsPerHeap))
			, m_DescriptorSize(0)
			, m_pDevice(nullptr)
		{}

		void Init(ID3D12Device* pDevice)
		{
			m_pDevice = pDevice;
			m_DescriptorSize = pDevice->GetDescriptorHandleIncrementSize(m_Desc.Type);
		}

		HeapOffset AllocateHeapSlot(HeapIndex& outIndex)
		{
			std::unique_lock Lock(CritSect);
			if (0 == m_FreeHeaps.size())
			{
				AllocateHeap();
			}
			wconstraint(0 != m_FreeHeaps.size());
			auto Head = m_FreeHeaps.begin();
			outIndex = *Head;
			SHeapEntry& HeapEntry = m_Heaps[outIndex];
			wconstraint(0 != HeapEntry.m_FreeList.size());
			SFreeRange& Range = HeapEntry.m_FreeList.front();
			HeapOffset Ret = { Range.Start };
			Range.Start += m_DescriptorSize;

			if (Range.Start == Range.End)
			{
				HeapEntry.m_FreeList.erase(HeapEntry.m_FreeList.begin());
				if (0 == HeapEntry.m_FreeList.size())
				{
					m_FreeHeaps.erase(Head);
				}
			}
			return Ret;
		}

		void FreeHeapSlot(HeapOffset Offset, HeapIndex index)
		{
			std::unique_lock Lock(CritSect);
			SHeapEntry& HeapEntry = m_Heaps[index];

			SFreeRange NewRange =
			{
				Offset.ptr,
				Offset.ptr + m_DescriptorSize
			};

			bool bFound = false;
			for (auto Node = HeapEntry.m_FreeList.begin();
				Node != HeapEntry.m_FreeList.end() && !bFound;
				Node = ++Node)
			{
				SFreeRange& Range = *Node;
				wconstraint(Range.Start < Range.End);
				if (Range.Start == Offset.ptr + m_DescriptorSize)
				{
					Range.Start = Offset.ptr;
					bFound = true;
				}
				else if (Range.End == Offset.ptr)
				{
					Range.End += m_DescriptorSize;
					bFound = true;
				}
				else
				{
					wconstraint(Range.End < Offset.ptr || Range.Start > Offset.ptr);
					if (Range.Start > Offset.ptr)
					{
						HeapEntry.m_FreeList.insert(Node,NewRange);
						bFound = true;
					}
				}
			}

			if (!bFound)
			{
				if (0 == HeapEntry.m_FreeList.size())
				{
					m_FreeHeaps.push_back(index);
				}
				HeapEntry.m_FreeList.push_back(NewRange);
			}
		}

	private: // Methods
		void AllocateHeap()
		{
			COMPtr<ID3D12DescriptorHeap> Heap;
			CheckHResult(m_pDevice->CreateDescriptorHeap(&m_Desc, IID_PPV_ARGS(&Heap.ReleaseAndGetRef())));
			D3D::Debug(Heap,"FD3D12OfflineDescriptorManager Descriptor Heap");

			HeapOffset HeapBase = Heap->GetCPUDescriptorHandleForHeapStart();
			wconstraint(HeapBase.ptr != 0);

			// Allocate and initialize a single new entry in the map
			m_Heaps.resize(m_Heaps.size() + 1);
			SHeapEntry& HeapEntry = m_Heaps.back();
			HeapEntry.m_FreeList.push_back({ HeapBase.ptr,
				HeapBase.ptr + m_Desc.NumDescriptors * m_DescriptorSize });
			HeapEntry.m_Heap = Heap;
			m_FreeHeaps.push_back(static_cast<HeapIndex>(m_Heaps.size() - 1));
		}

	private: // Members
		const D3D12_DESCRIPTOR_HEAP_DESC m_Desc;
		uint32 m_DescriptorSize;
		ID3D12Device* m_pDevice; // weak-ref

		THeapMap m_Heaps;
		std::list<HeapIndex> m_FreeHeaps;
		std::mutex CritSect;
	};


	class OnlineHeap : public DeviceChild,public SingleNodeGPUObject
	{
	public:
		OnlineHeap(NodeDevice* Device, GPUMaskType Node, bool CanLoopAround, DescriptorCache* _Parent = nullptr);
		virtual ~OnlineHeap() { }

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotHandle(uint32 Slot) const { return{ CPUBase.ptr + Slot * DescriptorSize }; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotHandle(uint32 Slot) const { return{ GPUBase.ptr + Slot * DescriptorSize }; }

		inline const uint32 GetDescriptorSize() const { return DescriptorSize; }

		const D3D12_DESCRIPTOR_HEAP_DESC& GetDesc() const { return Desc; }

		// Call this to reserve descriptor heap slots for use by the command list you are currently recording. This will wait if
		// necessary until slots are free (if they are currently in use by another command list.) If the reservation can be
		// fulfilled, the index of the first reserved slot is returned (all reserved slots are consecutive.) If not, it will 
		// throw an exception.
		bool CanReserveSlots(uint32 NumSlots);

		uint32 ReserveSlots(uint32 NumSlotsRequested);

		void SetNextSlot(uint32 NextSlot);

		ID3D12DescriptorHeap* GetHeap()
		{
			return Heap.Get();
		}

		void SetParent(DescriptorCache* InParent) { Parent = InParent; }

		// Roll over behavior depends on the heap type
		virtual bool RollOver() = 0;
		virtual void NotifyCurrentCommandList(CommandListHandle& CommandListHandle);

		virtual uint32 GetTotalSize()
		{
			return Desc.NumDescriptors;
		}

		static const uint32 HeapExhaustedValue = uint32(-1);
	protected:
		DescriptorCache* Parent;

		CommandListHandle CurrentCommandList;


		// Handles for manipulation of the heap
		uint32 DescriptorSize;
		D3D12_CPU_DESCRIPTOR_HANDLE CPUBase;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUBase;

		// This index indicate where the next set of descriptors should be placed *if* there's room
		uint32 NextSlotIndex;

		// Indicates the last free slot marked by the command list being finished
		uint32 FirstUsedSlot;

		// Keeping this ptr around is basically just for lifetime management
		COMPtr<ID3D12DescriptorHeap> Heap;

		// Desc contains the number of slots and allows for easy recreation
		D3D12_DESCRIPTOR_HEAP_DESC Desc;

		const bool bCanLoopAround;
	};

	class GlobalOnlineHeap : public OnlineHeap
	{
	public:
		GlobalOnlineHeap(NodeDevice* Device, GPUMaskType Node)
			: OnlineHeap(Device, Node, false,nullptr)
			, bUniqueDescriptorTablesAreDirty(false)
		{ }

		void Init(uint32 TotalSize, D3D12_DESCRIPTOR_HEAP_TYPE Type);

		void ToggleDescriptorTablesDirtyFlag(bool Value) { bUniqueDescriptorTablesAreDirty = Value; }
		bool DescriptorTablesDirty() { return bUniqueDescriptorTablesAreDirty; }
		SamplerSet& GetUniqueDescriptorTables() { return UniqueDescriptorTables; }
		std::mutex& GetCriticalSection() { return CriticalSection; }

		bool RollOver();
	private:
		SamplerSet UniqueDescriptorTables;
		bool bUniqueDescriptorTablesAreDirty;

		std::mutex CriticalSection;
	};

	struct OnlineHeapBlock
	{
	public:
		OnlineHeapBlock(uint32 _BaseSlot, uint32 _Size) :
			BaseSlot(_BaseSlot), Size(_Size), SizeUsed(0), bFresh(true) {};
		OnlineHeapBlock() : BaseSlot(0), Size(0), SizeUsed(0), bFresh(true) {}

		CLSyncPoint SyncPoint;
		uint32 BaseSlot;
		uint32 Size;
		uint32 SizeUsed;
		// Indicates that this has never been used in a Command List before
		bool bFresh;
	};

	class SubAllocatedOnlineHeap : public OnlineHeap
	{
	public:
		struct SubAllocationDesc
		{
			SubAllocationDesc() :ParentHeap(nullptr), BaseSlot(0), Size(0) {};
			SubAllocationDesc(GlobalOnlineHeap* _ParentHeap, uint32 _BaseSlot, uint32 _Size) :
				ParentHeap(_ParentHeap), BaseSlot(_BaseSlot), Size(_Size) {};

			GlobalOnlineHeap* ParentHeap;
			uint32 BaseSlot;
			uint32 Size;
		};

		SubAllocatedOnlineHeap(NodeDevice* Device, GPUMaskType Node, DescriptorCache* Parent) :
			OnlineHeap(Device, Node,false, Parent) {};

		void Init(SubAllocationDesc _Desc);

		// Specializations
		bool RollOver();
		void NotifyCurrentCommandList(CommandListHandle& CommandListHandle) override;

		virtual uint32 GetTotalSize() final override
		{
			return CurrentSubAllocation.Size;
		}
	private:

		std::queue<OnlineHeapBlock> DescriptorBlockPool;
		SubAllocationDesc SubDesc;

		OnlineHeapBlock CurrentSubAllocation;
	};

	class ThreadLocalOnlineHeap :public OnlineHeap
	{
	public:
		ThreadLocalOnlineHeap(NodeDevice* Device, GPUMaskType Node, DescriptorCache* _Parent)
			: OnlineHeap(Device, Node, true, _Parent)
		{ }

		bool RollOver();

		void NotifyCurrentCommandList(CommandListHandle& CommandListHandle) override;

		void Init(uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type);

	private:
		struct SyncPointEntry
		{
			CLSyncPoint SyncPoint;
			uint32 LastSlotInUse;

			SyncPointEntry() : LastSlotInUse(0)
			{}

			SyncPointEntry(const SyncPointEntry& InSyncPoint) : SyncPoint(InSyncPoint.SyncPoint), LastSlotInUse(InSyncPoint.LastSlotInUse)
			{}

			SyncPointEntry& operator = (const SyncPointEntry& InSyncPoint)
			{
				SyncPoint = InSyncPoint.SyncPoint;
				LastSlotInUse = InSyncPoint.LastSlotInUse;

				return *this;
			}
		};
		std::queue<SyncPointEntry> SyncPoints;

		struct PoolEntry
		{
			COMPtr<ID3D12DescriptorHeap> Heap;
			CLSyncPoint SyncPoint;

			PoolEntry()
			{}

			PoolEntry(const PoolEntry& InPoolEntry) : Heap(InPoolEntry.Heap), SyncPoint(InPoolEntry.SyncPoint)
			{}

			PoolEntry& operator = (const PoolEntry& InPoolEntry)
			{
				Heap = InPoolEntry.Heap;
				SyncPoint = InPoolEntry.SyncPoint;
				return *this;
			}
		};
		PoolEntry Entry;
		std::queue<PoolEntry> ReclaimPool;
	};

	class DescriptorCache : public DeviceChild,public SingleNodeGPUObject
	{
	protected:
		CommandContext* CmdContext;
	public:
		OnlineHeap* GetCurrentViewHeap() { return CurrentViewHeap; }
		OnlineHeap* GetCurrentSamplerHeap() { return CurrentSamplerHeap; }

		DescriptorCache(GPUMaskType Node);

		~DescriptorCache()
		{
			if (LocalViewHeap) { delete(LocalViewHeap); }
		}

		inline ID3D12DescriptorHeap* GetViewDescriptorHeap()
		{
			return CurrentViewHeap->GetHeap();
		}

		inline ID3D12DescriptorHeap* GetSamplerDescriptorHeap()
		{
			return CurrentSamplerHeap->GetHeap();
		}

		// Checks if the specified descriptor heap has been set on the current command list.
		bool IsHeapSet(ID3D12DescriptorHeap* const pHeap) const
		{
			return (pHeap == pPreviousViewHeap) || (pHeap == pPreviousSamplerHeap);
		}

		// Notify the descriptor cache every time you start recording a command list.
		// This sets descriptor heaps on the command list and indicates the current fence value which allows
		// us to avoid querying DX12 for that value thousands of times per frame, which can be costly.
		void NotifyCurrentCommandList(CommandListHandle& CommandListHandle);

		// ------------------------------------------------------
		// end Descriptor Slot Reservation stuff

		// null views
		DescriptorHandleSRV* pNullSRV;
		DescriptorHandleRTV* pNullRTV;
		DescriptorHandleUAV* pNullUAV;

		std::shared_ptr<SamplerState> pDefaultSampler;

		void SetVertexBuffers(VertexBufferCache& Cache);
		void SetRenderTargets(RenderTargetView** RenderTargetViewArray, uint32 Count, DepthStencilView* DepthStencilTarget);

		template <ShaderType ShaderStage>
		void SetUAVs(const RootSignature* RootSignature, UnorderedAccessViewCache& Cache, const UAVSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);

		template <ShaderType ShaderStage>
		void SetSamplers(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);

		template <ShaderType ShaderStage>
		void SetSRVs(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);

		template <ShaderType ShaderStage>
		void SetConstantBuffers(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask);

		void SetStreamOutTargets(ResourceHolder** Buffers, uint32 Count, const uint32* Offsets);

		bool HeapRolledOver(D3D12_DESCRIPTOR_HEAP_TYPE Type);
		void HeapLoopedAround(D3D12_DESCRIPTOR_HEAP_TYPE Type);
		void Init(NodeDevice* InParent, CommandContext* InCmdContext, uint32 InNumLocalViewDescriptors, uint32 InNumSamplerDescriptors, SubAllocatedOnlineHeap::SubAllocationDesc& SubHeapDesc);
		void Clear();
		void BeginFrame();
		void EndFrame();
		void GatherUniqueSamplerTables();

		bool SwitchToContextLocalViewHeap(CommandListHandle& CommandListHandle);
		bool SwitchToContextLocalSamplerHeap();
		bool SwitchToGlobalSamplerHeap();

		std::vector<UniqueSamplerTable>& GetUniqueTables() { return UniqueTables; }

		inline bool UsingGlobalSamplerHeap() const { return bUsingGlobalSamplerHeap; }
		SamplerSet& GetLocalSamplerSet() { return LocalSamplerSet; }

	private:
		// Sets the current descriptor tables on the command list and marks any descriptor tables as dirty if necessary.
		// Returns true if one of the heaps actually changed, false otherwise.
		bool SetDescriptorHeaps();

		// The previous view and sampler heaps set on the current command list.
		ID3D12DescriptorHeap* pPreviousViewHeap;
		ID3D12DescriptorHeap* pPreviousSamplerHeap;

		OnlineHeap* CurrentViewHeap;
		OnlineHeap* CurrentSamplerHeap;

		ThreadLocalOnlineHeap* LocalViewHeap;
		ThreadLocalOnlineHeap LocalSamplerHeap;
		SubAllocatedOnlineHeap SubAllocatedViewHeap;

		SamplerMap SamplerMap;

		std::vector<UniqueSamplerTable> UniqueTables;

		SamplerSet LocalSamplerSet;
		bool bUsingGlobalSamplerHeap;

		uint32 NumLocalViewDescriptors;
	};
}
