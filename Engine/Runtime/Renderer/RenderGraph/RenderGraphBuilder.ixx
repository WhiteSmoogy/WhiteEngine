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

	using RGPassHandle = RGHandle<RGPass, uint16>;
	using RGPassRegistry = RGHandleRegistry<RGPassHandle>;

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

	class RGBuilder
	{
	public:
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

	private:
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

	private:
		RGAllocator& Allocator;

		RGPassRegistry Passes;
		RGViewRegistry Views;
		RGConstBufferRegistry CBufferers;
		RGBufferRegistry Buffers;
	};
}