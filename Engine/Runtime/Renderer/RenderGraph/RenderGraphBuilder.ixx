module;
#include "RenderInterface/ICommandList.h"

export module RenderGraph:builder;

import :fwd;
import :definition;
import :resource;

export namespace RenderGraph
{
	using platform::Render::ComputeCommandList;

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
			:Name(std::move(InName)), ParameterStruct(InParameterStruct),Flag(InFlag)
		{}

		const char* GetName() const
		{
			return Name.GetName();
		}
	private:
		RGEventName Name;
		RGParameterStruct ParameterStruct;
		ERGPassFlags Flag;
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

	class RGBuilder:RGAllocatorScope
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
		RGTConstBufferRef<TBufferStruct> CreateCBuffer(const TBufferStruct* Struct)
		{
			auto cb = CBufferers.Allocate<RGTConstBuffer<TBufferStruct>>(Allocator, Struct, __func__);

			return cb;
		}

		template<typename TBufferStruct>
		RGTConstBufferRef<TBufferStruct> CreateCBuffer()
		{
			return CreateCBuffer(AllocParameters<TBufferStruct>());
		}

		RGBufferRef CreateBuffer(const RGBufferDesc& Desc, const char* Name, ERGBufferFlags Flags = ERGBufferFlags::None)
		{
			auto Buffer = Buffers.Allocate(Allocator, Name, Desc , Flags);
			return Buffer;
		}

		template<typename TBufferStruct>
		TBufferStruct* AllocParameters()
		{
			return Allocator.Alloc<TBufferStruct>();
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

		white::ref_ptr<RGPooledBuffer> ToExternal(RGBufferRef Buffer)
		{
			if (!Buffer->bExternal)
			{
				Buffer->bExternal = true;

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

		RGBufferRef RegisterExternal(const white::ref_ptr<RGPooledBuffer>& External, ERGBufferFlags Flags)
		{
			const char* Name = External->Name;
			if (!Name)
			{
				Name = "External";
			}

			return RegisterExternal(External, Name, Flags);
		}

		RGBufferRef RegisterExternal(const white::ref_ptr<RGPooledBuffer>& External, const char* Name, ERGBufferFlags Flags)
		{
			if (auto FoundBuffer = FindExternal(External.get()))
			{
				return FoundBuffer;
			}

			auto Buffer = Buffers.Allocate(Allocator, Name, External->Desc, Flags);
			SetRObject(Buffer, External, GetProloguePassHandle());

			Buffer->bExternal = true;
			ExternalBuffers.emplace(Buffer->GetRObject(), Buffer);

			return Buffer;
		}

		void Execute();
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

			return Pass;
		}

		void SetRObject(RGBuffer* Buffer, const white::ref_ptr<RGPooledBuffer>& Pooled, RGPassHandle PassHandle)
		{
			auto BufferObj = Pooled->GetRObject();

			Buffer->RealObj = BufferObj;
			Buffer->Allocation = Pooled;
			Buffer->FirstPass = PassHandle;
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
	};
}