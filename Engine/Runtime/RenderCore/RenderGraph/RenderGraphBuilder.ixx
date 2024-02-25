module;
#include "RenderInterface/ICommandList.h"
#include "Runtime/RenderCore/Dispatch.h"
#include "RenderInterface/DeviceCaps.h"
#include "WBase/wmathtype.hpp"
#include "Core/Container/vector.hpp"

#include <span>

export module RenderGraph:builder;

import "WBase/cassert.h";
import :fwd;

import :definition;
import :resource;
import :resourcepool;

export namespace RenderGraph
{
	using platform::Render::ComputeCommandList;
	using platform::Render::EPipeline;
	using platform::Render::EAccessHint;
	using platform::Render::ShaderBaseType;

	enum class ERGPassFlags
	{
		None = 0,
		Raster = 1 << 0,
		Compute = 1 << 1,
		AsyncCompute = 1 << 2,
		Copy = 1 << 3,
		NeverCull = 1 << 4,
	};

	const char* RGPassTypeToString(ERGPassFlags PassType)
	{
		if (white::has_anyflags(PassType, ERGPassFlags::Raster))
		{
			if (white::has_anyflags(PassType, ERGPassFlags::NeverCull))
				return "Raster | NeverCull";
			return "Raster";
		}
		else if (white::has_anyflags(PassType, ERGPassFlags::Compute))
		{
			if (white::has_anyflags(PassType, ERGPassFlags::NeverCull))
		   		return "Compute | NeverCull";
			return "Compute";
		}
		else if (white::has_anyflags(PassType, ERGPassFlags::AsyncCompute))
		{
			if (white::has_anyflags(PassType, ERGPassFlags::NeverCull))
				return "AsyncCompute | NeverCull";
			return "AsyncCompute";
		}
		else if (white::has_anyflags(PassType, ERGPassFlags::Copy))
		{
			if (white::has_anyflags(PassType, ERGPassFlags::NeverCull))
				return "Copy | NeverCull";
			return "Copy";
		}
		return "Unknown";
	}

	class RGPass
	{
	public:
		RGPass(RGEventName&& InName, RGParameterStruct InParameterStruct, ERGPassFlags InFlag)
			:Name(std::move(InName)), ParameterStruct(InParameterStruct), Flags(InFlag)
			, Pipeline(white::has_anyflags(Flags, ERGPassFlags::AsyncCompute)? EPipeline::Compute:EPipeline::Graphics)
		{}

		const char* GetName() const
		{
			return Name.GetName();
		}

		RGParameterStruct GetParameters() const
		{
			return ParameterStruct;
		}

		bool IsCulled() const
		{
			return bCulled;
		}

	protected:
		virtual void Execute(ComputeCommandList& CmdList) {}

		struct BufferState
		{
			BufferState() = default;

			BufferState(RGBufferRef InBuffer)
			: Buffer(InBuffer)
			{}

			RGBufferRef Buffer = nullptr;
			RGSubresourceState State;
			RGSubresourceState* MergeState = nullptr;
			uint16 ReferenceCount = 0;
		};
	protected:
		RGEventName Name;
		RGParameterStruct ParameterStruct;
		ERGPassFlags Flags;

		EPipeline Pipeline;
			 
		RGPassHandle Handle;

		RGPassHandle GraphicsForkPass;
		RGPassHandle GraphicsJoinPass;
		RGPassHandle PrologueBarrierPass;
		RGPassHandle EpilogueBarrierPass;

		RGArray<RGPass*> ResourcesToBegin;
		RGArray<RGPass*> ResourcesToEnd;


		RGArray<BufferState> BufferStates;
		
		RGArray<RGViewHandle> Views;
		RGArray<RGConstBufferHandle> CBuffers;

		bool bEmptyParameters : 1 = 0;
		bool bRenderPassOnlyWrites : 1 = 0;
		bool bHasExternalOutputs : 1 = 0;
		bool bSentinel :1  = 0;
		bool bCulled : 1 = 0;

		RGPassHandleArray Producers;
		RGPassHandleArray CrossPipelineConsumers;
		RGPassHandle CrossPipelineProducer;

		friend RGPassRegistry;
		friend RGBuilder;
	};

	using RGPassRef = RGPass*;

	class RGSentinelPass : public RGPass
	{
	public:
		RGSentinelPass(RGEventName&& Name, ERGPassFlags InPassFlagsToAdd = ERGPassFlags::None)
			:RGPass(std::move(Name), RGParameterStruct((uint8_t*)nullptr,nullptr),white::enum_or(ERGPassFlags::NeverCull ,InPassFlagsToAdd))
		{
			bSentinel = 1;
		}
	};

	template <typename ParameterStructType, typename ExecuteLambdaType>
	class RGTLambdaPass
		: public RGPass
	{
		// Verify that the amount of stuff captured by the pass lambda is reasonable.
		static constexpr int32 kMaximumLambdaCaptureSize = 1024;
		static_assert(sizeof(ExecuteLambdaType) <= kMaximumLambdaCaptureSize, "The amount of data of captured for the pass looks abnormally high.");

		template <typename T>
		struct TLambdaTraits
			: TLambdaTraits<decltype(&T::operator())>
		{};
		template <typename ReturnType, typename ClassType, typename ArgType>
		struct TLambdaTraits<ReturnType(ClassType::*)(ArgType&) const>
		{
			using TCommandList = ArgType;
			using TRGPass = void;
		};
		template <typename ReturnType, typename ClassType, typename ArgType>
		struct TLambdaTraits<ReturnType(ClassType::*)(ArgType&)>
		{
			using TCommandList = ArgType;
			using TRGPass = void;
		};
		template <typename ReturnType, typename ClassType, typename ArgType1, typename ArgType2>
		struct TLambdaTraits<ReturnType(ClassType::*)(const ArgType1*, ArgType2&) const>
		{
			using TCommandList = ArgType2;
			using TRGPass = ArgType1;
		};
		template <typename ReturnType, typename ClassType, typename ArgType1, typename ArgType2>
		struct TLambdaTraits<ReturnType(ClassType::*)(const ArgType1*, ArgType2&)>
		{
			using TCommandList = ArgType2;
			using TRGPass = ArgType1;
		};
		using TCommandList = typename TLambdaTraits<ExecuteLambdaType>::TCommandList;
		using TRGPass = typename TLambdaTraits<ExecuteLambdaType>::TRGPass;

	public:
		RGTLambdaPass(
			RGEventName&& InName,
			const ShaderParametersMetadata* InParameterMetadata,
			const ParameterStructType* InParameterStruct,
			ERGPassFlags InPassFlags,
			ExecuteLambdaType&& InExecuteLambda)
			: RGPass(std::move(InName), RGParameterStruct(InParameterStruct, InParameterMetadata), InPassFlags)
			, ExecuteLambda(std::move(InExecuteLambda))
		{
		}

	private:
		template<class T>
		void ExecuteLambdaFunc(ComputeCommandList& RHICmdList)
		{
			if constexpr (std::is_same_v<T, RGPass>)
			{
				ExecuteLambda(this, static_cast<TCommandList&>(RHICmdList));
			}
			else
			{
				ExecuteLambda(static_cast<TCommandList&>(RHICmdList));
			}
		}

		void Execute(ComputeCommandList& RHICmdList) override
		{
			ExecuteLambdaFunc<TRGPass>(static_cast<TCommandList&>(RHICmdList));
		}

		ExecuteLambdaType ExecuteLambda;
	};

	enum class ERGBuilderFlags
	{
		None = 0,

		/** Allows the builder to parallelize execution of passes. Without this flag, all passes execute on the render thread. */
		AllowParallelExecute = 1 << 0
	};

	inline void GetPassAccess(ERGPassFlags PassFlags, EAccessHint& SRVAccess, EAccessHint& UAVAccess)
	{
		SRVAccess = EAccessHint::None;
		UAVAccess = EAccessHint::None;

		if (white::has_anyflags(PassFlags, ERGPassFlags::Raster))
		{
			SRVAccess = white::enum_or(SRVAccess, EAccessHint::SRVGraphics);
			UAVAccess = white::enum_or(UAVAccess, EAccessHint::UAVGraphics);
		}

		if (white::has_anyflags(PassFlags, white::enum_or(ERGPassFlags::AsyncCompute,ERGPassFlags::Compute)))
		{
			SRVAccess =  white::enum_or(SRVAccess, EAccessHint::SRVCompute);
			UAVAccess =  white::enum_or(SRVAccess, EAccessHint::UAVCompute);
		}

		if (white::has_anyflags(PassFlags, ERGPassFlags::Copy))
		{
			SRVAccess = white::enum_or(SRVAccess, EAccessHint::CopySrc);
		}
	}

	template <typename TAccessFunction>
	void EnumerateBufferAccess(RGParameterStruct PassParameters, ERGPassFlags PassFlags, TAccessFunction AccessFunction)
	{
		EAccessHint SRVAccess, UAVAccess;
		GetPassAccess(PassFlags, SRVAccess, UAVAccess);

		PassParameters.EnumerateBuffers([&](RGParameter Parameter)
			{
				auto baseType = Parameter.GetShaderBaseType();

				if (white::has_anyflags(baseType, SBT_SRV))
				{
					if (auto SRV = Parameter.GetAsBufferSRV())
					{
						auto Buffer = SRV->GetParent();
						auto BufferAccess = SRVAccess;

						AccessFunction(SRV, Buffer, BufferAccess);
					}
				}
				else if (white::has_anyflags(baseType, SBT_UAV))
				{
					if (auto UAV = Parameter.GetAsBufferUAV())
					{
						auto Buffer = UAV->GetParent();

						AccessFunction(UAV, Buffer, UAVAccess);
					}
				}
				else if(baseType == SBT_BUFFER)
				{
					if (auto Access = Parameter.GetAsBufferAccess())
					{
						AccessFunction(nullptr, Access.GetBuffer(), Access.GetAccess());
					}
				}
			}
		);
	}

	RGViewHandle GetHandleIfNoUAVBarrier(RGViewRef Resource)
	{
		if (Resource && (Resource->Type == ERGViewType::BufferUAV || Resource->Type == ERGViewType::TextureUAV))
		{
			if (white::has_anyflags(static_cast<RGUnorderedAccessViewRef>(Resource)->Flags, ERGUnorderedAccessViewFlags::SkipBarrier))
			{
				return Resource->GetHandle();
			}
		}
		return RGViewHandle::Null;
	}

	EAccessHint MakeValidAccess(EAccessHint AccessOld, EAccessHint AccessNew)
	{
		const EAccessHint AccessUnion = white::enum_or(AccessOld ,AccessNew);
		const EAccessHint NonMergeableAccessMask = white::enum_compl(Caps.MergeableAccessMask);

		// Return the union of new and old if they are okay to merge.
		if (!white::has_anyflags(AccessUnion, NonMergeableAccessMask))
		{
			return IsWritableAccess(AccessUnion) ? white::enum_and(AccessUnion , white::enum_compl(EAccessHint::ReadOnlyExclusiveMask)) : AccessUnion;
		}

		// Keep the old one if it can't be merged.
		if (white::has_anyflags(AccessOld, NonMergeableAccessMask))
		{
			return AccessOld;
		}

		// Replace with the new one if it can't be merged.
		return AccessNew;
	}

	class RGBuilder :RGAllocatorScope
	{
	public:
		RGBuilder(CommandListImmediate& InCmdList, RGEventName InName = {}, ERGBuilderFlags Flags = ERGBuilderFlags::None)
			:CmdList(InCmdList), BuilderName(InName)
			, bParallelSetupEnabled(white::has_anyflags(Flags, ERGBuilderFlags::AllowParallelExecute))
			, bParallelExecuteEnabled(white::has_anyflags(Flags, ERGBuilderFlags::AllowParallelExecute))
		{
			AddProloguePass();
		}

		RGBuilder(const RGBuilder&) = delete;
		~RGBuilder()
		{

		}

		RGBufferUAVRef CreateUAV(const RGBufferUAVDesc& Desc, ERGUnorderedAccessViewFlags InFlags = ERGUnorderedAccessViewFlags::None)
		{
			auto UAV = Views.Allocate<RGBufferUAV>(Allocator, Desc.Buffer->Name, Desc, InFlags);

			return UAV;
		}

		RGBufferSRVRef CreateSRV(const RGBufferSRVDesc& Desc)
		{
			auto SRV = Views.Allocate<RGBufferSRV>(Allocator, Desc.Buffer->Name, Desc);

			return SRV;
		}

		template<typename TBufferStruct>
		RGTConstBufferRef<TBufferStruct> CreateCBuffer()
		{
			return CreateCBuffer(AllocParameters<TBufferStruct>(), __func__);
		}

		template<typename TBufferStruct>
		RGConstBufferRef CreateCBuffer(std::span<TBufferStruct> span)
		{
			auto ParametersSize = static_cast<uint32>(span.size_bytes());
			auto Parameters = Allocator.Alloc(ParametersSize, alignof(TBufferStruct));

			std::memcpy(Parameters, span.data(), ParametersSize);

			return CreateCBuffer(Parameters, __func__).Get();
		}

		RGBufferRef CreateBuffer(const RGBufferDesc& Desc, const char* Name, ERGBufferFlags Flags = ERGBufferFlags::None)
		{
			auto Buffer = Buffers.Allocate(Allocator, Name, Desc, Flags);
			return Buffer;
		}

		template<typename TBufferStruct>
		TBufferStruct* AllocParameters()
		{
			return Allocator.Alloc<TBufferStruct>();
		}

		template<typename TBufferStruct>
		std::span<TBufferStruct> AllocParameters(uint32 Count)
		{
			return std::span<TBufferStruct> {Allocator.AllocUninitialized<TBufferStruct>(Count), Count};
		}

		template <typename ParameterStructType, typename ExecuteLambdaType>
		RGPassRef AddPass(
			RGEventName&& Name,
			const ParameterStructType* Struct,
			ERGPassFlags Flags,
			ExecuteLambdaType&& Lambda
		) requires IsRGParameterStruct<ParameterStructType>
		{
			return AddPassInternal(std::move(Name), RGParameterStruct::GetStructMetadata<ParameterStructType>(), Struct, Flags, std::forward<ExecuteLambdaType>(Lambda));
		}

		template <typename ExecuteLambdaType>
		RGPassRef AddPass(
			RGEventName&& Name,
			const ShaderParametersMetadata* Metadata,
			const void* Struct,
			ERGPassFlags Flags,
			ExecuteLambdaType&& Lambda
		)
		{
			return AddPassInternal(std::move(Name), Metadata, Struct, Flags, std::forward<ExecuteLambdaType>(Lambda));
		}

		white::ref_ptr<RGPooledBuffer> ToExternal(RGBufferRef Buffer)
		{
			if (!Buffer->bExternal)
			{
				Buffer->bExternal = 1;
				Buffer->bForceNonTransient = 1;

				BeginResource(GetProloguePassHandle(), Buffer);

				ExternalBuffers.emplace(Buffer->GetRObject(), Buffer);
			}

			return Buffer->Allocation;
		}

		RGBufferRef FindExternal(GraphicsBuffer* ExternalBuffer) const
		{
			auto itr = ExternalBuffers.find(ExternalBuffer);
			if (itr != ExternalBuffers.end())
			{
				return itr->second;
			}
			return nullptr;
		}

		RGBufferRef FindExternal(RGPooledBuffer* ExternalBuffer) const
		{
			if (ExternalBuffer)
			{
				return FindExternal(ExternalBuffer->GetRObject());
			}
			return nullptr;
		}

		RGBufferRef RegisterExternal(const white::ref_ptr<RGPooledBuffer>& External, ERGBufferFlags Flags = ERGBufferFlags::None)
		{
			const char* Name = External->Name;
			if (!Name)
			{
				Name = "External";
			}

			return RegisterExternal(External, Name, Flags);
		}

		RGBufferRef RegisterExternal(const white::ref_ptr<RGPooledBuffer>& External, const char* Name, ERGBufferFlags Flags = ERGBufferFlags::None)
		{
			if (auto FoundBuffer = FindExternal(External.get()))
			{
				return FoundBuffer;
			}

			auto Buffer = Buffers.Allocate(Allocator, Name, External->Desc, Flags);
			SetRObject(Buffer, External, GetProloguePassHandle());

			Buffer->bExternal = 1;
			ExternalBuffers.emplace(Buffer->GetRObject(), Buffer);

			return Buffer;
		}

		void Execute()
		{
			SetupEmptyPass(EpiloguePass = Passes.Allocate<RGSentinelPass>(Allocator, RGEventName("Graph Epilogue")));

			const auto ProloguePassHandle = GetProloguePassHandle();
			const auto EpiloguePassHandle = GetEpiloguePassHandle();

			Compile();
			CompilePassBarriers();

			for (const auto& Pair : ExternalBuffers)
			{
				auto Buffer = Pair.second;

				if (Buffer->IsCulled())
				{
					EndResource(ProloguePassHandle, Buffer, 0);
				}
			}

			for (auto PassHandle = ProloguePassHandle; PassHandle <= EpiloguePassHandle; ++PassHandle)
			{
				auto* Pass = Passes[PassHandle];

				if (!Pass->bCulled)
				{
					BeginResources(Pass, PassHandle);
					EndResources(Pass, PassHandle);
				}
			}
		}

		void Dump(const char* graphfilename)
		{
			static struct GraphVizStyle
			{
				char const* rank_dir{ "TB" };
				struct
				{
					char const* name{ "helvetica" };
					int32       size{ 10 };
				} font;
				struct
				{
					struct
					{
						char const* executed{ "orange" };
						char const* culled{ "lightgray" };
					} pass;
					struct
					{
						char const* imported{ "lightsteelblue" };
						char const* transient{ "skyblue" };
					} resource;
					struct
					{
						char const* read{ "olivedrab3" };
						char const* write{ "orangered" };
					} edge;
				} color;
			} style;

			struct GraphViz
			{
				std::string defaults;
				std::string declarations;
				std::string dependencies;
			} graphviz;

			graphviz.defaults += std::format("graph [style=invis, rankdir=\"{}\", ordering=out, splines=spline]\n", style.rank_dir);
			graphviz.defaults += std::format("node [shape=record, fontname=\"{}\", fontsize={}, margin=\"0.2,0.03\"]\n", style.font.name, style.font.size);

			std::unordered_set<RGBuffer*> declared_buffers;

			auto DeclareBuffer = [&declared_buffers, &graphviz, this](RGBuffer* buffer)
				{
					if (!declared_buffers.contains(buffer))
					{
						graphviz.declarations += std::format("B{} ", buffer->Handle.GetIndex());
						std::string label = std::format("<{}<br/>dimension: Buffer<br/>size: {} bytes>",
							buffer->Name, buffer->Desc.GetSize());
						graphviz.declarations += std::format("[shape=\"box\", style=\"filled\",fillcolor={}, label={}] \n", buffer->bExternal ? style.color.resource.imported : style.color.resource.transient, label);
						declared_buffers.insert(buffer);
					}
				};

			const auto ProloguePassHandle = GetProloguePassHandle();
			const auto EpiloguePassHandle = GetEpiloguePassHandle();

			for (auto PassHandle = ProloguePassHandle + 1; PassHandle < EpiloguePassHandle; ++PassHandle)
			{
				auto* Pass = Passes[PassHandle];

				graphviz.declarations += std::format("P{} ", Pass->Handle.GetIndex());
				std::string label = std::format("<{}<br/> type: {}<br/> culled: {}>", Pass->GetName(), RGPassTypeToString(Pass->Flags), Pass->IsCulled() ? "Yes" : "No");
				graphviz.declarations += std::format("[shape=\"ellipse\", style=\"rounded,filled\",fillcolor={}, label={}] \n",
					Pass->IsCulled() ? style.color.pass.culled : style.color.pass.executed, label);

				std::string read_dependencies = "{";
				std::string write_dependencies = "{";

				for (auto& BufferState : Pass->BufferStates)
				{
					auto name = std::format("B{} ", BufferState.Buffer->Handle.GetIndex());
					DeclareBuffer(BufferState.Buffer);

					if (IsWritableAccess(BufferState.State.Access))
					{
						write_dependencies += name;
						write_dependencies += ",";
					}
					else
					{
						read_dependencies += name;
						read_dependencies += ",";
					}
				}

				if (read_dependencies.back() == ',') read_dependencies.pop_back();
				read_dependencies += "}";
				if (write_dependencies.back() == ',') write_dependencies.pop_back();
				write_dependencies += "}";

				graphviz.dependencies += std::format("{}->P{} [color=olivedrab3]\n", read_dependencies, Pass->Handle.GetIndex());
				graphviz.dependencies += std::format("P{}->{} [color=orangered]\n", Pass->Handle.GetIndex(), write_dependencies);
			}

			std::ofstream graph_file(graphfilename);
			graph_file << "digraph RenderGraph{ \n";
			graph_file << graphviz.defaults << "\n";
			graph_file << graphviz.declarations << "\n";
			graph_file << graphviz.dependencies << "\n";
			graph_file << "}";
			graph_file.close();
		}
	private:
		void Compile()
		{
			const auto ProloguePassHandle = GetProloguePassHandle();
			const auto EpiloguePassHandle = GetEpiloguePassHandle();

			const uint32 CompilePassCount = Passes.Num();

			if (bParallelSetupEnabled)
			{
				//Flush([RGPass* Pass]{SetupPassResources(Pass);});
			}

			auto bCullPasses = GRGCullPasses;

			if (bCullPasses)
				CullPassStack.reserve(CompilePassCount);

			if (bCullPasses || AsyncComputePassCount > 0)
			{
				if (!bParallelSetupEnabled)
				{
					for (auto PassHandle = ProloguePassHandle + 1; PassHandle < EpiloguePassHandle; ++PassHandle)
					{
						SetupPassDependencies(Passes[PassHandle]);
					}
				}

				const auto AddLastProducersToCullStack = [&](const RGProducerStatesByPipeline& LastProducers)
				{
					for (const auto& LastProducer : LastProducers)
					{
						if (LastProducer.Pass)
						{
							CullPassStack.emplace_back(LastProducer.Pass->Handle);
						}
					}
				};

				for(auto& pair :ExternalBuffers)
				{
					AddLastProducersToCullStack(pair.second->LastProducer);
				}
			}

			if (bCullPasses)
			{
				CullPassStack.emplace_back(EpiloguePassHandle);

				// Mark the epilogue pass as culled so that it is traversed.
				EpiloguePass->bCulled = 1;

				// Manually mark the prologue passes as not culled.
				ProloguePass->bCulled = 0;

				while (!CullPassStack.empty())
				{
					auto* Pass = Passes[white::pop(CullPassStack)];

					if (Pass->bCulled)
					{
						Pass->bCulled = 0;

						white::append(CullPassStack,Pass->Producers);
					}
				}

				for (auto PassHandle = ProloguePassHandle + 1; PassHandle < EpiloguePassHandle; ++PassHandle)
				{
					auto* Pass = Passes[PassHandle];

					if (!Pass->bCulled)
					{
						continue;
					}


					for (auto& PassState : Pass->BufferStates)
					{
						PassState.Buffer->ReferenceCount -= PassState.ReferenceCount;
					}
				}
			}
		}

		void CompilePassBarriers()
		{

		}

		void EndResource(RGPassHandle PassHandle, RGBufferRef Buffer, uint32 ReferenceCount)
		{
			wassume(Buffer->ReferenceCount != RGViewableResource::DeallocatedReferenceCount);
			wassume(Buffer->ReferenceCount >= ReferenceCount);
			Buffer->ReferenceCount -= ReferenceCount;

			if (Buffer->ReferenceCount == 0)
			{
				if (Buffer->bTransient)
				{
					Buffer->TransientBuffer.reset();
				}
				else
				{
					Buffer->Allocation = nullptr;
				}

				Buffer->LastPass = PassHandle;
				Buffer->ReferenceCount = RGViewableResource::DeallocatedReferenceCount;
			}
		}

		void BeginResources(RGPass* ResourcePass, RGPassHandle ExecutePassHandle)
		{
			for (auto PassToBegin : ResourcePass->ResourcesToBegin)
			{
				for (const auto& PassState : PassToBegin->BufferStates)
				{
					BeginResource(ExecutePassHandle, PassState.Buffer);
				}

				for (auto CBufferHandle : PassToBegin->CBuffers)
				{
					if (auto* CBuffer = CBuffers[CBufferHandle]; !CBuffer->bQueuedForCreate)
					{
						CBuffer->bQueuedForCreate = 1;
						CBuffersToCreate.emplace_back(CBufferHandle);
					}
				}

				for (auto ViewHandle : PassToBegin->Views)
				{
					InitRObject(Views[ViewHandle]);
				}
			}
		}

		void EndResources(RGPass* ResourcePass, RGPassHandle ExecutePassHandle)
		{

		}


		void AddProloguePass()
		{
			ProloguePass = SetupEmptyPass(Passes.Allocate<RGSentinelPass>(Allocator, RGEventName("Graph Prologue (Graphics)")));
		}

		RGPassHandle GetProloguePassHandle() const
		{
			return RGPassHandle(0);
		}

		RGPassHandle GetEpiloguePassHandle() const
		{
			return Passes.Last();
		}

		template <typename ParameterStructType, typename ExecuteLambdaType>
		RGPassRef AddPassInternal(
			RGEventName&& Name,
			const ShaderParametersMetadata* Metadata,
			const ParameterStructType* Struct,
			ERGPassFlags Flags,
			ExecuteLambdaType&& Lambda
		)
		{
			using PassType = RGTLambdaPass<ParameterStructType, ExecuteLambdaType>;

			auto Pass = Allocator.AllocNoDestruct<PassType>(
				std::move(Name),
				Metadata,
				Struct,
				Flags,
				std::forward<ExecuteLambdaType>(Lambda)
			);

			Passes.Insert(Pass);
			SetupParameterPass(Pass);

			return nullptr;
		}

		template<typename TBufferStruct>
		RGTConstBufferRef<TBufferStruct> CreateCBuffer(TBufferStruct* Struct,const char* Name)
		{
			auto cb = CBuffers.Allocate<RGTConstBuffer<TBufferStruct>>(Allocator, Struct, Name);

			return { .CBuffer = cb };
		}

		void SetRObject(RGBuffer* Buffer, const white::ref_ptr<RGPooledBuffer>& Pooled, RGPassHandle PassHandle)
		{
			auto BufferObj = Pooled->GetRObject();

			Buffer->RealObj = BufferObj;
			Buffer->Allocation = Pooled;
			Buffer->FirstPass = PassHandle;
		}

		void SetRObject(RGBuffer* Buffer, GraphicsBufferRef TransientBuffer, RGPassHandle PassHandle)
		{
			Buffer->RealObj = TransientBuffer.get();
			Buffer->bTransient = 1;
			Buffer->FirstPass = PassHandle;
			Buffer->TransientBuffer = TransientBuffer;
			Buffer->State = Allocator.AllocNoDestruct<RGSubresourceState>();
		}

		void InitRObject(RGViewRef View)
		{
			if (View->HasRObject())
			{
				return;
			}

			if (View->Type == ERGViewType::BufferSRV)
			{
				
			}
			else if (View->Type == ERGViewType::BufferUAV)
			{
			}
		}

		bool IsTransientInternal(RGViewableResource* Resource)
		{
			if (Resource->bForceNonTransient)
			{
				return false;
			}

			return true;
		}

		bool IsTransient(RGBufferRef Buffer)
		{
			if (!IsTransientInternal(Buffer))
				return false;

			if (white::has_anyflags(Buffer->Desc.Usage, EAccessHint::DrawIndirect))
				return false;

			return white::has_anyflags(Buffer->Desc.Usage, EAccessHint::UAV);
		}

		void BeginResource(RGPassHandle PassHandle, RGBufferRef Buffer)
		{
			if (Buffer->HasRObject())
				return;

			//transient create
			if (IsTransient(Buffer))
			{
				ResourceCreateInfo CreateInfo(Buffer->Name);

				GraphicsBufferRef TransientBuffer = platform::Render::CreateBuffer(Buffer::Usage::SingleFrame, Buffer->Desc.Usage, Buffer->Desc.GetSize(), Buffer->Desc.BytesPerElement, CreateInfo);
				SetRObject(Buffer, TransientBuffer, PassHandle);
			}

			if (!Buffer->bTransient)
			{
				auto Alignment = Buffer->bQueuedForUpload ? ERGPooledBufferAlignment::PowerOfTwo : ERGPooledBufferAlignment::Page;

				SetRObject(Buffer, GRenderGraphResourcePool.FindFreeBuffer(Buffer->Desc, Buffer->Name, Alignment), PassHandle);
			}
		}

		RGPass* SetupParameterPass(RGPass* Pass)
		{
			SetupPassInternals(Pass);

			if (bParallelSetupEnabled)
			{
				throw white::unimplemented();
			}
			else
			{
				SetupPassResources(Pass);
			}

			return Pass;
		}

		void SetupPassInternals(RGPass* Pass)
		{
			const auto PassHandle = Pass->Handle;
			const auto PassFlags = Pass->Flags;
			const auto PassPipeline = Pass->Pipeline;

			Pass->GraphicsForkPass = PassHandle;
			Pass->GraphicsJoinPass = PassHandle;
			Pass->PrologueBarrierPass = PassHandle;
			Pass->EpilogueBarrierPass = PassHandle;

			if (Pass->Pipeline == EPipeline::Graphics)
			{
				Pass->ResourcesToBegin.emplace_back(Pass);
				Pass->ResourcesToEnd.emplace_back(Pass);
			}

			AsyncComputePassCount += white::has_anyflags(PassFlags, ERGPassFlags::AsyncCompute) ? 1 : 0;
			RasterPassCount += white::has_anyflags(PassFlags, ERGPassFlags::Raster) ? 1 : 0;
		}


		void SetupPassResources(RGPass* Pass)
		{
			const auto PassParameters = Pass->GetParameters();
			const auto PassHandle = Pass->Handle;
			const auto PassFlags = Pass->Flags;
			const auto PassPipeline = Pass->Pipeline;

			bool bRenderPassOnlyWrites = true;

			const auto TryAddView = [&](RGViewRef View)
				{
					if (View && View->LastPass != PassHandle)
					{
						View->LastPass = PassHandle;
						Pass->Views.emplace_back(View->Handle);
					}
				};

			Pass->Views.reserve(PassParameters.GetBufferParameterCount() + PassParameters.GetTextureParameterCount());

			EnumerateBufferAccess(PassParameters, PassFlags, [&](RGViewRef BufferView, RGBufferRef Buffer, EAccessHint Access)
				{
					TryAddView(BufferView);

					const auto NoUAVBarrierHandle = GetHandleIfNoUAVBarrier(BufferView);

					RGPass::BufferState* PassState;

					if (Buffer->LastPass != PassHandle)
					{
						Buffer->LastPass = PassHandle;
						Buffer->PassStateIndex = static_cast<int>(Pass->BufferStates.size());

						PassState = &Pass->BufferStates.emplace_back(Buffer);
					}
					else
					{
						PassState = &Pass->BufferStates[Buffer->PassStateIndex];
					}

					PassState->ReferenceCount++;
					PassState->State.Access = MakeValidAccess(PassState->State.Access, Access);
					PassState->State.NoUAVBarrierFilter.AddHandle(NoUAVBarrierHandle);
					PassState->State.SetPass(PassPipeline, PassHandle);

					if (IsWritableAccess(Access))
					{
						bRenderPassOnlyWrites = false;

						// When running in parallel this is set via MarkResourcesAsProduced. We also can't touch this as its a bitfield and not atomic.
						if (!bParallelSetupEnabled)
						{
							Buffer->bProduced = true;
						}
					}
				});

			Pass->bEmptyParameters = Pass->BufferStates.empty();
			Pass->bRenderPassOnlyWrites = bRenderPassOnlyWrites;

			Pass->CBuffers.reserve(PassParameters.GetCBufferCount());
			PassParameters.EnumerateCBuffers([&](RGUniformBufferBinding UniformBuffer)
				{
					if (auto cbuffer = UniformBuffer.GetCBuffer())
					{
						Pass->CBuffers.emplace_back(cbuffer->Handle);
					}
				});

			if (bParallelSetupEnabled)
			{

			}
		}

		RGPass* SetupEmptyPass(RGPass* Pass)
		{
			Pass->bEmptyParameters = true;
			SetupPassInternals(Pass);
			SetupAuxiliaryPasses(Pass);
			return Pass;
		}

		void SetupAuxiliaryPasses(RGPass* Pass)
		{
		}

		void SetupPassDependencies(RGPass* Pass)
		{
			for (auto& PassState : Pass->BufferStates)
			{
				auto Buffer = PassState.Buffer;
				const auto& SubresourceState = PassState.State;

				Buffer->ReferenceCount += PassState.ReferenceCount;

				RGProducerState ProducerState;
				ProducerState.Pass = Pass;
				ProducerState.Access = SubresourceState.Access;
				ProducerState.NoUAVBarrierHandle = SubresourceState.NoUAVBarrierFilter.GetUniqueHandle();

				AddCullingDependency(Buffer->LastProducer, ProducerState, Pass->Pipeline);
			}

			const bool bCullPasses = GRGCullPasses;
			Pass->bCulled = bCullPasses;

			if (bCullPasses && (Pass->bHasExternalOutputs || white::has_anyflags(Pass->Flags, ERGPassFlags::NeverCull)))
			{
				CullPassStack.emplace_back(Pass->Handle);
			}
		}

		void AddCullingDependency(RGProducerStatesByPipeline& LastProducers,const RGProducerState& NextState, EPipeline NextPipeline)
		{
			for (auto LastPipeline : GetPipelines())
			{
				auto & LastProducer = LastProducers[LastPipeline];

				if (LastProducer.Access != EAccessHint::None)
				{
					auto LastProducerPass = LastProducer.Pass;

					if (LastPipeline != NextPipeline)
					{
						auto MultiPipelineUAVMask = white::enum_and(EAccessHint::UAV, Caps.MultiPipelineMergeableAccessMask);

						if (white::has_anyflags(NextState.Access, MultiPipelineUAVMask) && SkipUAVBarrier(LastProducer.NoUAVBarrierHandle, NextState.NoUAVBarrierHandle))
						{
							LastProducerPass = LastProducer.PassIfSkipUAVBarrier;
						}
					}

					if (LastProducerPass)
					{
						AddPassDependency(LastProducerPass, NextState.Pass);
					}
				}
			}

			if (IsWritableAccess(NextState.Access))
			{
				auto& LastProducer = LastProducers[NextPipeline];

				// A separate producer pass is tracked for UAV -> UAV dependencies that are skipped. Consider the following scenario:
				//
				//     Graphics:       A   ->    B         ->         D      ->     E       ->        G         ->            I
				//                   (UAV)   (SkipUAV0)           (SkipUAV1)    (SkipUAV1)          (SRV)                   (UAV2)
				//
				// Async Compute:                           C                ->               F       ->         H
				//                                      (SkipUAV0)                        (SkipUAV1)           (SRV)
				//
				// Expected Cross Pipe Dependencies: [A -> C], C -> D, [B -> F], F -> G, E -> H, F -> I. The dependencies wrapped in
				// braces are only introduced properly by tracking a different producer for cross-pipeline skip UAV dependencies, which
				// is only updated if skip UAV is inactive, or if transitioning from one skip UAV set to another (or another writable resource).

				if (LastProducer.NoUAVBarrierHandle.IsNull())
				{
					if (NextState.NoUAVBarrierHandle.IsNull())
					{
						// Assigns the next producer when no skip UAV sets are active.
						LastProducer.PassIfSkipUAVBarrier = NextState.Pass;
					}
				}
				else if (LastProducer.NoUAVBarrierHandle != NextState.NoUAVBarrierHandle)
				{
					// Assigns the last producer in the prior skip UAV barrier set when moving out of a skip UAV barrier set.
					LastProducer.PassIfSkipUAVBarrier = LastProducer.Pass;
				}

				LastProducer.Access = NextState.Access;
				LastProducer.Pass = NextState.Pass;
				LastProducer.NoUAVBarrierHandle = NextState.NoUAVBarrierHandle;
			}
		}

		void AddPassDependency(RGPass* Producer, RGPass* Consumer)
		{
			auto& Producers = Consumer->Producers;

			if (std::find(Producers.begin(), Producers.end(), Producer->Handle) == Producers.end())
			{
				const auto BinarySearchOrAdd = [](RGPassHandleArray& Range, RGPassHandle Handle)
					{
						const auto LowerBound = std::lower_bound(Range.begin(),Range.end(), Handle);
						if (LowerBound != Range.end())
						{
							if (*LowerBound == Handle)
							{
								return;
							}
						}
						Range.insert(LowerBound, Handle);
					};

				BinarySearchOrAdd(Producer->CrossPipelineConsumers, Consumer->Handle);

				if (Consumer->CrossPipelineProducer.IsNull() || Producer->Handle > Consumer->CrossPipelineProducer)
				{
					Consumer->CrossPipelineProducer = Producer->Handle;
				}

				Producers.emplace_back(Producer->Handle);
			}
		}

	private:
		CommandListImmediate& CmdList;
		const RGEventName BuilderName;

		bool bParallelSetupEnabled;
		bool bParallelExecuteEnabled;

		RGPassRegistry Passes;
		RGViewRegistry Views;
		RGConstBufferRegistry CBuffers;
		RGBufferRegistry Buffers;

		RGArray<RGConstBufferHandle> CBuffersToCreate;


		std::unordered_map<
			GraphicsBuffer*,
			RGBufferRef,
			std::hash<GraphicsBuffer*>,
			std::equal_to<GraphicsBuffer*>,
			RGSTLAllocator<std::pair<GraphicsBuffer* const, RGBufferRef>>> ExternalBuffers;

		uint32 AsyncComputePassCount;
		uint32 RasterPassCount;

		RGPassRef ProloguePass;
		RGPassRef EpiloguePass;

		RGArray<RGPassHandle> CullPassStack;
	};
}

export namespace ComputeShaderUtils
{
	using namespace RenderGraph;

	inline void ValidateGroupCount(const white::math::int3& GroupCount)
	{
		wassume(GroupCount.x <= Caps.MaxDispatchThreadGroupsPerDimension.x);
		wassume(GroupCount.y <= Caps.MaxDispatchThreadGroupsPerDimension.y);
		wassume(GroupCount.z <= Caps.MaxDispatchThreadGroupsPerDimension.z);
	}

	template<typename TShaderClass>
	inline RGPassRef AddPass(
		RGBuilder& GraphBuilder,
		RGEventName&& PassName,
		ERGPassFlags PassFlags,
		const Render::ShaderRef<TShaderClass>& ComputeShader,
		const Render::ShaderParametersMetadata* ParametersMetadata,
		typename TShaderClass::Parameters* Parameters,
		white::math::int3 GroupCount)
	{
		WAssert(
			white::has_anyflags(PassFlags,white::enum_or(ERGPassFlags::Compute,ERGPassFlags::AsyncCompute)) &&
			!white::has_anyflags(PassFlags, white::enum_or(ERGPassFlags::Copy,ERGPassFlags::Raster)), "AddPass only supports 'Compute' or 'AsyncCompute'.");

		ValidateGroupCount(GroupCount);

		return GraphBuilder.AddPass(
			std::move(PassName),
			ParametersMetadata,
			Parameters,
			PassFlags,
			[ParametersMetadata, Parameters, ComputeShader, GroupCount](platform::Render::ComputeCommandList& CmdList)
			{
				 ComputeShaderUtils::Dispatch(CmdList, ComputeShader, *Parameters, GroupCount);
			});
	}

	template <typename TShaderClass>
	inline RGPassRef AddPass(
		RGBuilder& GraphBuilder,
		RGEventName&& PassName,
		const Render::ShaderRef<TShaderClass>& ComputeShader,
		typename TShaderClass::Parameters* Parameters,
		white::math::int3 GroupCount)
	{
		auto* ParametersMetadata = TShaderClass::Parameters::TypeInfo::GetStructMetadata();
		return AddPass(GraphBuilder, std::move(PassName), ERGPassFlags::Compute, ComputeShader, ParametersMetadata, Parameters, GroupCount);
	}
}