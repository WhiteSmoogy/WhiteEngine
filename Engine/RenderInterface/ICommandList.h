#pragma once

#include "ICommandContext.h"
#include "IRayTracingGeometry.h"
#include "IRayTracingScene.h"
#include "Runtime/MemStack.h"
#include "Core/Threading/ManualResetEvent.h"

//Command List definitions for queueing up & executing later.

namespace platform::Render {
	class CommandListBase;
	class Display;

	enum class EPipeline: uint32
	{
		Graphics = EAccessHint::Graphics,
		Compute = EAccessHint::Compute,
	};

	inline constexpr uint32 GetPipelineIndex(EPipeline Pipeline)
	{
		switch (Pipeline)
		{
		case EPipeline::Graphics:
			return 0;
		default:
			return 1;
		}
	}

	constexpr uint32 GetPipelineCount()
	{
		return 2;
	}

	template<typename ElementType>
	class PipelineArray
	{
	public:
		PipelineArray()
			:Elements()
		{}

		ElementType& operator[](EPipeline Pipeline)
		{
			return Elements[GetPipelineIndex(Pipeline)];
		}

		const ElementType& operator[](EPipeline Pipeline) const
		{
			return Elements[GetPipelineIndex(Pipeline)];
		}
	private:
		ElementType Elements[GetPipelineCount()];
	};

	enum class EResourceTransitionFlags
	{
		None = 0,

		MaintainCompression = 1 << 0, // Specifies that the transition should not decompress the resource, allowing us to read a compressed resource directly in its compressed state.
		Discard = 1 << 1, // Specifies that the data in the resource should be discarded during the transition - used for transient resource acquire when the resource will be fully overwritten
		Clear = 1 << 2, // Specifies that the data in the resource should be cleared during the transition - used for transient resource acquire when the resource might not be fully overwritten

		Last = Clear,
		Mask = (Last << 1) - 1
	};

	struct CommandListContext
	{};

	struct CommandBase
	{
		CommandBase* Next = nullptr;

		virtual void ExecuteAndDestruct(CommandListBase& CmdList, CommandListContext& Context) = 0;
	};

	template <typename TCmd>
	struct LadmbdaCommand : public CommandBase
	{
		TCmd Cmd;

		LadmbdaCommand(TCmd&& InCmd)
			:Cmd(InCmd)
		{}

		void ExecuteAndDestruct(CommandListBase& CmdList, CommandListContext& Context) override final
		{
			Cmd(CmdList);
			Cmd.~TCmd();
		}
	};

	template <typename TCmd> 
	struct TCommand : public CommandBase
	{
		void ExecuteAndDestruct(CommandListBase& CmdList, CommandListContext& Context) override final
		{
			TCmd* ThisCmd = static_cast<TCmd*>(this);

			ThisCmd->Execute(CmdList);
			ThisCmd->~TCmd();
		}
	};

	class CommandListBase : white::noncopyable
	{
	public:
		CommandListBase();

		void Flush();

		void SetContext(CommandContext* InContext)
		{
			Context = InContext;
			ComputeContext = InContext;
		}

		CommandContext& GetContext()
		{
			return *Context;
		}

		ComputeContext& GetComputeContext()
		{
			return *ComputeContext;
		}

		void Reset();

		void* Alloc(int32 AllocSize, int32 Alignment)
		{
			return MemManager.Alloc(AllocSize, Alignment);
		}

		void* AllocBuffer(uint32 NumBytes, const void* Buffer)
		{
			void* NewBuffer = Alloc(NumBytes, 16);
			memcpy(NewBuffer, Buffer, NumBytes);
			return NewBuffer;
		}

		template<typename CharT>
		CharT* AllocString(const CharT* Name)
		{
			int32 Len =static_cast<int32>(std::char_traits<CharT>::length(Name) + 1);
			CharT* NameCopy = (CharT*)Alloc(Len * (int32)sizeof(CharT), (int32)sizeof(CharT));
			std::char_traits<CharT>::copy(NameCopy, Name,Len);
			return NameCopy;
		}

		void* AllocCommand(int32 AllocSize, int32 Aligment)
		{
			CommandBase* Result = (CommandBase*)Alloc(AllocSize,Aligment);

			*CommandLink = Result;
			CommandLink = &Result->Next;

			return Result;
		}

		template<typename TCmd>
		void* AllocCommand()
		{
			return AllocCommand(sizeof(TCmd), alignof(TCmd));
		}

		void ExchangeCmdList(CommandListBase& CmdList)
		{
			byte storage[sizeof(CommandListBase)];
			std::memcpy(storage, &CmdList, sizeof(storage));
			std::memcpy(&CmdList, this, sizeof(storage));
			std::memcpy(this, storage, sizeof(storage));

			if (CommandLink == &CmdList.Root)
			{
				CommandLink = &Root;
			}
			if (CmdList.CommandLink == &Root)
			{
				CmdList.CommandLink = &CmdList.Root;
			}
		}

		void CopyContext(CommandListBase& CmdList)
		{
			Context = CmdList.Context;
			ComputeContext = CmdList.ComputeContext;
		}

		bool IsExecuting() { return bExecuting; }
	protected:
		CommandBase* Root;
		CommandBase** CommandLink;

		CommandContext* Context;
		ComputeContext* ComputeContext;

		WhiteEngine::MemStackBase MemManager;

		friend class CommandListIterator;
		friend class CommandListExecutor;

		struct PSOContext
		{
			uint32 CachedNumSimultanousRenderTargets = 0;
			std::array<RenderTarget, MaxSimultaneousRenderTargets> CachedRenderTargets;
			DepthRenderTarget CachedDepthStencilTarget;

		} PSOContext;

		bool bExecuting = false;
	};

#define CL_ALLOC_COMMAND(CmdList, ...) new ( (CmdList).AllocCommand(sizeof(__VA_ARGS__), alignof(__VA_ARGS__)) ) __VA_ARGS__

	class ComputeCommandList : public CommandListBase
	{
	public:
		template<typename TCmd>
		void InsertCommand(TCmd&& Cmd)
		{
			new (AllocCommand(sizeof(LadmbdaCommand<TCmd>), alignof(LadmbdaCommand<TCmd>))) LadmbdaCommand<TCmd> {std::forward<TCmd>(Cmd)};
		}

		template<typename TCmd,typename... Args>
		void InsertCommand(TCmd&& Cmd,Args&&... args)
		{
			new (AllocCommand(sizeof(LadmbdaCommand<TCmd>), alignof(LadmbdaCommand<TCmd>))) LadmbdaCommand<TCmd> {TCmd(std::forward<Args>(args)...)};
		}

		void SetShaderSampler(const ComputeHWShader* Shader, uint32 SamplerIndex, const TextureSampleDesc& Desc)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetShaderSampler(Shader, SamplerIndex, Desc);
			});
		}

		void SetShaderTexture(const ComputeHWShader* Shader, uint32 TextureIndex, Texture* Texture)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetShaderTexture(Shader, TextureIndex, Texture);
			});
		}

		void SetShaderResourceView(const ComputeHWShader* Shader, uint32 TextureIndex, ShaderResourceView* Texture)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetShaderResourceView(Shader, TextureIndex, Texture);
				});
		}

		void SetShaderParameter(const ComputeHWShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
		{
			InsertCommand([=, UseValue = AllocBuffer(NumBytes, NewValue)](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetShaderParameter(Shader, BufferIndex, BaseIndex, NumBytes, UseValue);
				});
		}

		void SetUAVParameter(const ComputeHWShader* Shader, uint32 UAVIndex, UnorderedAccessView* UAV)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetUAVParameter(Shader, UAVIndex, UAV);
				});
		}

		void SetUAVParameter(const ComputeHWShader* Shader, uint32 UAVIndex, UnorderedAccessView* UAV, uint32 InitialCount)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetUAVParameter(Shader, UAVIndex, UAV, InitialCount);
				});
		}

		void SetComputePipelineState(ComputePipelineState* pso)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetComputePipelineState(pso);
				});
		}

		void SetShaderConstantBuffer(const ComputeHWShader* Shader, uint32 BaseIndex, ConstantBuffer* Buffer)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetShaderConstantBuffer(Shader, BaseIndex, Buffer);
				});
		}

		void DispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().DispatchComputeShader(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				});
		}
	};


	class CommandList :public ComputeCommandList
	{
	public:
		void BeginRenderPass(const RenderPassInfo& Info, const char* Name)
		{
			auto NameCopy = AllocString(Name);
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().BeginRenderPass(Info, NameCopy);
			});

			RenderTargetsInfo RTInfo(Info);
			
			CacheActiveRenderTargets(RTInfo.NumColorRenderTargets, RTInfo.ColorRenderTarget, &RTInfo.DepthStencilRenderTarget);
		}

		void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetViewport(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
			});
		}

		void SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetScissorRect(bEnable, MinX, MinY, MaxX, MaxY);
				});
		}

		void SetVertexBuffer(uint32 slot, GraphicsBuffer* VertexBuffer)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetVertexBuffer(slot, VertexBuffer);
				});
		}

		void SetIndexBuffer(platform::Render::GraphicsBuffer* IndexBuffer)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetIndexBuffer(IndexBuffer);
				});
		}

		void SetGraphicsPipelineState(GraphicsPipelineState* pso)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetGraphicsPipelineState(pso);
				});
		}

		template<THardwareShader T>
		void SetShaderSampler(T* Shader, uint32 SamplerIndex, const TextureSampleDesc& Desc)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderSampler(Shader, SamplerIndex, Desc);
				});
		}

		using ComputeCommandList::SetShaderSampler;

		template<THardwareShader T>
		void SetShaderTexture(T* Shader, uint32 TextureIndex, Texture* Texture)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderTexture(Shader, TextureIndex, Texture);
				});
		}

		using ComputeCommandList::SetShaderTexture;

		template<THardwareShader T>

		void SetShaderResourceView(T* Shader, uint32 TextureIndex, ShaderResourceView* SRV)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderResourceView(Shader, TextureIndex, SRV);
				});
		}

		using ComputeCommandList::SetShaderResourceView;

		template<THardwareShader T>
		void SetShaderConstantBuffer(T* Shader, uint32 BaseIndex, ConstantBuffer* Buffer)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderConstantBuffer(Shader, BaseIndex, Buffer);
				});
		}

		void DrawIndexedPrimitive(GraphicsBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().DrawIndexedPrimitive(IndexBuffer, BaseVertexIndex, FirstInstance, NumVertices, StartIndex, NumPrimitives, NumInstances);
				});
		}

		template<THardwareShader T>
		void SetShaderParameter(T* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
		{
			InsertCommand([=,UseValue=AllocBuffer(NumBytes,NewValue)](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderParameter(Shader, BufferIndex, BaseIndex, NumBytes, UseValue);
				});
		}

		using ComputeCommandList::SetShaderParameter;

		void DrawPrimitive(uint32 BaseVertexIndex, uint32 FirstInstance, uint32 NumPrimitives, uint32 NumInstances)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().DrawPrimitive(BaseVertexIndex, FirstInstance, NumPrimitives, NumInstances);
				});
		}

		void DrawIndirect(CommandSignature* Sig, uint32 MaxCmdCount, GraphicsBuffer* IndirectBuffer, uint32 BufferOffset, GraphicsBuffer* CountBuffer, uint32 CountBufferOffset)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().DrawIndirect(Sig, MaxCmdCount, IndirectBuffer, BufferOffset, CountBuffer, CountBufferOffset);
				});
		}

		void FillRenderTargetsInfo(GraphicsPipelineStateInitializer& GraphicsPSOInit)
		{
			GraphicsPSOInit.RenderTargetsEnabled = PSOContext.CachedNumSimultanousRenderTargets;
			for (uint32 i = 0; i < GraphicsPSOInit.RenderTargetsEnabled; ++i)
			{
				if (PSOContext.CachedRenderTargets[i].Texture)
				{
					GraphicsPSOInit.RenderTargetFormats[i] = PSOContext.CachedRenderTargets[i].Texture->GetFormat();
				}
				else
				{
					GraphicsPSOInit.RenderTargetFormats[i] = EF_Unknown;
				}

				if (GraphicsPSOInit.RenderTargetFormats[i] != EF_Unknown)
				{
					GraphicsPSOInit.NumSamples = PSOContext.CachedRenderTargets[i].Texture->GetSampleCount();
				}
			}

			if (PSOContext.CachedDepthStencilTarget.Texture)
			{
				GraphicsPSOInit.DepthStencilTargetFormat = PSOContext.CachedDepthStencilTarget.Texture->GetFormat();
			}
			else 
			{
				GraphicsPSOInit.DepthStencilTargetFormat = EF_Unknown;
			}
		}

		//Ray-Tracing
		void BuildAccelerationStructure(RayTracingGeometry* geometry)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				geometry->BuildAccelerationStructure(CmdList.GetContext());
				});
		}

		void BuildAccelerationStructure(std::shared_ptr<RayTracingScene> scene)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				scene->BuildAccelerationStructure(CmdList.GetContext());
				});
		}

		void PushEvent(const char16_t* Name, platform::FColor Color)
		{
			InsertCommand([=,Name= AllocString(Name)](CommandListBase& CmdList) {
				CmdList.GetComputeContext().PushEvent(Name, Color);
				});
		}

		void PopEvent()
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().PopEvent();
				});
		}

		void BeginFrame();
		void EndFrame();

		void Present(Display* display);

		SRVRIRef CreateShaderResourceView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format = EF_Unknown);
		UAVRIRef CreateUnorderedAccessView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format = EF_Unknown);
	protected:
		

		void CacheActiveRenderTargets(
			uint32 NewNumSimultaneousRenderTargets,
			const RenderTarget* RenderTargets,
			const DepthRenderTarget* DepthStencilTarget
		)
		{
			PSOContext.CachedNumSimultanousRenderTargets = NewNumSimultaneousRenderTargets;

			for (uint32 RTIdx = 0; RTIdx < PSOContext.CachedNumSimultanousRenderTargets; ++RTIdx)
			{
				PSOContext.CachedRenderTargets[RTIdx] = RenderTargets[RTIdx];
			}

			PSOContext.CachedDepthStencilTarget = DepthStencilTarget?*DepthStencilTarget: DepthRenderTarget();
		}
	};

	class CommandListImmediate : public CommandList
	{
	public:
		using RIFenceType = white::threading::manual_reset_event;
		using RIFenceTypeRef = std::shared_ptr<RIFenceType>;


		void ImmediateFlush();

		RIFenceTypeRef ThreadFence();
	};

	class CommandListExecutor
	{
	public:
		void ExecuteList(CommandListBase& CmdList);

		static inline CommandListImmediate& GetImmediateCommandList();
	private:
		void ExecuteInner(CommandListBase& CmdList);

		static void Execute(CommandListBase& CmdList);

		CommandListImmediate Immediate;
	};

	extern CommandListExecutor GCommandList;

	CommandListImmediate& CommandListExecutor::GetImmediateCommandList()
	{
		return GCommandList.Immediate;
	}

	class CommandListIterator
	{
	public:
		CommandListIterator(CommandListBase& CmdList)
			:CmdPtr(CmdList.Root),NumCommands(0)
		{
		}

		operator bool() const
		{
			return !!CmdPtr;
		}

		CommandListIterator& operator++()
		{
			CmdPtr = CmdPtr->Next;
			++NumCommands;
			return *this;
		}

		CommandBase* operator->()
		{
			return CmdPtr;
		}

	protected:
		CommandBase* CmdPtr;
		uint32 NumCommands;
	};

	bool extern GRenderInterfaceSupportCommandThread;

	inline bool IsRunningCommandInThread()
	{
		return GRenderInterfaceSupportCommandThread;
	}

	bool IsInRenderCommandThread();
}