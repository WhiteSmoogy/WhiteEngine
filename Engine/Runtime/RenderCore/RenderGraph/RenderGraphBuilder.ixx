module;
#include "RenderInterface/ICommandList.h"
#include "Runtime/RenderCore/Dispatch.h"
#include "RenderInterface/DeviceCaps.h"

#include <span>

export module RenderGraph:builder;

import "WBase/cassert.h";

import RenderGraphFwd;
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

	protected:
		virtual void Execute(ComputeCommandList& CmdList) {}

	private:
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

		RGArray<RGViewHandle> Views;

		friend RGPassRegistry;
		friend RGBuilder;
	};

	using RGPassRef = RGPass*;

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
			}
		);
	}

	class RGBuilder :RGAllocatorScope
	{
	public:
		RGBuilder(CommandListImmediate& InCmdList, RGEventName InName = {}, ERGBuilderFlags Flags = ERGBuilderFlags::None)
			:CmdList(InCmdList), BuilderName(InName)
			, bParallelSetupEnabled(white::has_anyflags(Flags, ERGBuilderFlags::AllowParallelExecute))
			, bParallelExecuteEnabled(white::has_anyflags(Flags, ERGBuilderFlags::AllowParallelExecute))
		{}

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

			auto cb = CBufferers.Allocate<RGConstBuffer>(Allocator, Parameters, ParametersSize,  __func__);

			return cb;
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
		{}
	private:
		RGPassHandle GetProloguePassHandle() const
		{
			return RGPassHandle(0);
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

			return Pass;
		}

		template<typename TBufferStruct>
		RGTConstBufferRef<TBufferStruct> CreateCBuffer(TBufferStruct* Struct,const char* Name)
		{
			auto cb = CBufferers.Allocate<RGTConstBuffer<TBufferStruct>>(Allocator, Struct, Name);

			return { .CBuffer = cb };
		}

		void SetRObject(RGBuffer* Buffer, const white::ref_ptr<RGPooledBuffer>& Pooled, RGPassHandle PassHandle)
		{
			auto BufferObj = Pooled->GetRObject();

			Buffer->RealObj = BufferObj;
			Buffer->Allocation = Pooled;
			Buffer->FirstPass = PassHandle;
		}

		void BeginResource(RGPassHandle PassHandle, RGBufferRef Buffer)
		{
			if (Buffer->HasRObject())
				return;

			//transient create

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
				});
		}

	private:
		CommandListImmediate& CmdList;
		const RGEventName BuilderName;

		bool bParallelSetupEnabled;
		bool bParallelExecuteEnabled;

		RGPassRegistry Passes;
		RGViewRegistry Views;
		RGConstBufferRegistry CBufferers;
		RGBufferRegistry Buffers;

		std::unordered_map<
			GraphicsBuffer*,
			RGBufferRef,
			std::hash<GraphicsBuffer*>,
			std::equal_to<GraphicsBuffer*>,
			RGSTLAllocator<std::pair<GraphicsBuffer* const, RGBufferRef>>> ExternalBuffers;

		uint32 AsyncComputePassCount;
		uint32 RasterPassCount;
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
			[ParametersMetadata, Parameters, ComputeShader, GroupCount](ComputeCommandList& CmdList)
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