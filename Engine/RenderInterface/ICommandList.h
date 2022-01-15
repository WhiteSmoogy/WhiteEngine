#pragma once

#include "ICommandContext.h"
#include "Runtime/Core/MemStack.h"

//Command List definitions for queueing up & executing later.

namespace platform::Render {
	class CommandListBase;

	struct CommandListContext
	{};

	struct CommandBase
	{
		CommandBase* Next = nullptr;

		virtual void ExecuteAndDestruct(CommandListBase& CmdList, CommandListContext& Context) = 0;
	};

	template <typename TCmd>
	struct Command : public CommandBase
	{
		TCmd Cmd;

		Command(TCmd&& InCmd)
			:Cmd(InCmd)
		{}

		void ExecuteAndDestruct(CommandListBase& CmdList, CommandListContext& Context) override final
		{
			Cmd(CmdList);
			this->~Command();
		}
	};

	class CommandListBase : white::noncopyable
	{
	public:
		CommandListBase();

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

		template<typename CharT>
		CharT* AllocString(const CharT* Name)
		{
			int32 Len = std::char_traits<CharT>::length(Name) + 1;
			CharT* NameCopy = (CharT*)Alloc(Len * (int32)sizeof(CharT), (int32)sizeof(CharT));
			std::char_traits<CharT>::::copy(NameCopy, Name,Len);
			return NameCopy;
		}

		void* AllocCommand(int32 AllocSize, int32 Aligment)
		{
			CommandBase* Result = (CommandBase*)::operator new(AllocSize, (std::align_val_t)Aligment);

			*CommandLink = Result;
			CommandLink = &Result->Next;
		}
	private:
		CommandBase** CommandLink;

		CommandContext* Context;
		ComputeContext* ComputeContext;

		WhiteEngine::MemStackBase MemManager;
	};

	class ComputeCommandList : public CommandListBase
	{
	public:
		template<typename TCmd>
		void InsertCommand(TCmd&& Cmd)
		{
			new (AllocCommand(sizeof(Command<TCmd>), alignof(Command<TCmd>))) Command<TCmd> {std::forward<TCmd>(Cmd)};
		}

		template<typename TCmd,typename... Args>
		void InsertCommand(TCmd&& Cmd,Args&&... args)
		{
			new (AllocCommand(sizeof(Command<TCmd>), alignof(Command<TCmd>))) Command<TCmd> {TCmd(std::forward<Args>(args)...)};
		}

		void SetShaderSampler(ComputeHWShader* Shader, uint32 SamplerIndex, const TextureSampleDesc& Desc)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetShaderSampler(Shader, SamplerIndex, Desc);
			});
		}

		void SetShaderTexture(ComputeHWShader* Shader, uint32 TextureIndex, Texture* Texture)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetShaderTexture(Shader, TextureIndex, Texture);
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

		void SetGraphicsPipelineState(GraphicsPipelineState* pso)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetGraphicsPipelineState(pso);
				});
		}

		template<typename THardwareShader>
		void SetShaderSampler(THardwareShader* Shader, uint32 SamplerIndex, const TextureSampleDesc& Desc)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderSampler(Shader, SamplerIndex, Desc);
				}):
		}

		using ComputeCommandList::SetShaderSampler;

		template<typename THardwareShader>
		void SetShaderTexture(THardwareShader* Shader, uint32 TextureIndex, Texture* Texture)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderTexture(Shader, TextureIndex, Texture);
				});
		}

		using ComputeCommandList::SetShaderTexture;

		template<typename THardwareShader>
		void SetShaderResourceView(THardwareShader* Shader, uint32 TextureIndex, ShaderResourceView* SRV)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderResourceView(Shader, TextureIndex, SRV);
				});
		}

		template<typename THardwareShader>
		void SetShaderConstantBuffer(THardwareShader* Shader, uint32 BaseIndex, GraphicsBuffer* Buffer)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderConstantBuffer(Shader, BaseIndex, Buffer);
				}):
		}

		void DrawIndexedPrimitive(GraphicsBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().DrawIndexedPrimitive(IndexBuffer, BaseVertexIndex, FirstInstance, NumVertices, StartIndex, NumPrimitives, NumInstances);
				});
		}

		void DrawPrimitive(uint32 BaseVertexIndex, uint32 FirstInstance, uint32 NumPrimitives, uint32 NumInstances)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().DrawPrimitive(BaseVertexIndex, FirstInstance, NumPrimitives, NumInstances);
				});
		}

		template<typename THardwareShader>
		void SetShaderParameter(THardwareShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetContext().SetShaderParameter(Shader, BufferIndex, BaseIndex, NumBytes, NewValue);
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

		void SetUAVParameter(ComputeHWShader* Shader, uint32 UAVIndex, UnorderedAccessView* UAV)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetUAVParameter(Shader, UAVIndex, UAV);
				});
		}

		void SetUAVParameter(ComputeHWShader* Shader, uint32 UAVIndex, UnorderedAccessView* UAV, uint32 InitialCount)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetUAVParameter(Shader, UAVIndex, UAV, InitialCount);
				});
		}

		void SetComputeShader(ComputeHWShader* Shader)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().SetComputeShader(Shader);
				});
		}

		void DispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
		{
			InsertCommand([=](CommandListBase& CmdList) {
				CmdList.GetComputeContext().DispatchComputeShader(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				});
		}

		void PushEvent(const char16_t* Name, platform::FColor Color)
		{
			InsertCommand([=](CommandListBase& CmdList) {
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
	protected:
		struct PSOContext
		{
			uint32 CachedNumSimultanousRenderTargets = 0;
			std::array<RenderTarget, MaxSimultaneousRenderTargets> CachedRenderTargets;
			DepthRenderTarget CachedDepthStencilTarget;

		} PSOContext;

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

	CommandList& GetCommandList();
}