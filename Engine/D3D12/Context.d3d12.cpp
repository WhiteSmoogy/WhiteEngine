#include "Context.h"

#include <WBase/pointer.hpp>
#include <WFramework/Core/WException.h>
#include <WFramework/WCLib/Logger.h>

#include "RenderInterface/ICommandList.h"
#include "RenderInterface/Effect/CopyEffect.h"
#include "RenderInterface/PipelineStateUtility.h"

#include "Convert.h"
#include "FrameBuffer.h"
#include "RayContext.h"
#include "NodeDevice.h"
#include "DirectStorage.h"

#if ENABLE_AFTER_MATH
#include <Aftermath/GFSDK_Aftermath.h>
#include <Aftermath/GFSDK_Aftermath_GpuCrashDump.h>
#include <Aftermath/GFSDK_Aftermath_GpuCrashDumpDecoding.h>

void GpuCrashDumpCallback(
	const void* pGpuCrashDump,
	const uint32_t gpuCrashDumpSize,
	void* pUserData);

#define AFTERMATH_CHECK_ERROR(FC)  [&](){\
GFSDK_Aftermath_Result _result = FC;\
WAssert(GFSDK_Aftermath_SUCCEED(_result), "Aftermath Error");\
}()

#if WFL_Win64
#pragma comment(lib,"Aftermath/lib/x64/GFSDK_Aftermath_Lib.x64.lib")
#endif
#endif

#define TEST_CODE 1
#if TEST_CODE
extern HWND g_hwnd;
#endif



namespace platform_ex::Windows::D3D12 {
	Device& GetDevice()
	{
		return Context::Instance().GetDevice();
	}

	NodeDevice* D3D12::GetDefaultNodeDevice()
	{
		return GetDevice().GetNodeDevice(0);
	}

	Context::Context()
		:adapter_list()
	{
#if ENABLE_AFTER_MATH
		if (GEnableNvidaiAfterMath)
		{
			GFSDK_Aftermath_EnableGpuCrashDumps(
				GFSDK_Aftermath_Version_API,
				GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
				GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
				GpuCrashDumpCallback,
				nullptr,
				nullptr,
				this
			);
		}
		else
#endif
		{
#ifndef NDEBUG
			COMPtr<ID3D12Debug> debug_ctrl;
			if (SUCCEEDED(D3D12::GetDebugInterface(COMPtr_RefParam(debug_ctrl, IID_ID3D12Debug)))) {
				WAssertNonnull(debug_ctrl);
				debug_ctrl->EnableDebugLayer();

				platform::Render::Caps.IsDebugLayerEnabled = true;
			}
#endif
		}
	}

	DXGI::Adapter & Context::DefaultAdapter()
	{
		return adapter_list.CurrentAdapter();
	}

	void D3D12::Context::SyncCPUGPU(bool force)
	{
		GetDefaultCommandContext()->FlushResourceBarriers();
		GetDefaultCommandContext()->FlushCommands();

		device->EndFrame();
	}

	const COMPtr<ID3D12GraphicsCommandList> & D3D12::Context::GetCommandList(Device::CommandType index) const
	{
		return d3d_cmd_lists[index];
	}

	std::mutex & D3D12::Context::GetCommandListMutex(Device::CommandType index)
	{
		return cmd_list_mutexs[index];
	}

	void D3D12::Context::SyncCommand(Device::CommandType type)
	{
		auto val = GetFence(type).Signal((QueueType)type);
		GetFence(type).WaitForFence(val);
	}

	void D3D12::Context::ResetCommand(Device::CommandType type)
	{
		CheckHResult(d3d_cmd_lists[type]->Reset(GetDevice().d3d_cmd_allocators[type].Get(), nullptr));
	}

	void Context::ExecuteUAVBarrier()
	{
		auto nodedvice = device->GetNodeDevice(0);
		auto& commandcontext = nodedvice->GetDefaultCommandContext();
		commandcontext.AddUAVBarrier();
	}

	void D3D12::Context::CommitCommandList(Device::CommandType type)
	{
		CheckHResult(d3d_cmd_lists[type]->Close());
		ID3D12CommandList* cmd_lists[] = { d3d_cmd_lists[type].Get() };
		device->GetNodeDevice(0)->GetQueue(QueueType::Direct).D3DCommandQueue->ExecuteCommandLists(1, cmd_lists);

		if (type == Device::CommandType::Command_Resource) {
			auto val = GetFence(type).Signal(QueueType::Direct);
			GetFence(type).WaitForFence(val);

			GetDevice().d3d_cmd_allocators[type]->Reset();
			ResetCommand(type);
		}
	}

	void Context::ContextEx(ID3D12Device * d3d_device, ID3D12CommandQueue * cmd_queue)
	{
		CheckHResult(d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			device->d3d_cmd_allocators[Device::Command_Resource].Get(), nullptr,
			COMPtr_RefParam(d3d_cmd_lists[Device::Command_Resource], IID_ID3D12GraphicsCommandList)));
		D3D::Debug(d3d_cmd_lists[Device::Command_Resource], "Resource_Command");

		D3D12_COMMAND_QUEUE_DESC queue_desc =
		{
			D3D12_COMMAND_LIST_TYPE_DIRECT, //Type
			0, //Priority
			D3D12_COMMAND_QUEUE_FLAG_NONE, //Flags
			0 //NodeMask
		};

		for (auto& fence : device->fences) {
			fence.reset(new Fence(device.get(), GPUMaskType::AllGPU()));
			fence->CreateFence();
		}

		device->GetNodeDevice(0)->Initialize();

		if (device->GetRayTracingDevice())
		{
			ray_context = std::make_shared<RayContext>(device.get(), this);
		}
	}

	Fence & Context::GetFence(Device::CommandType index)
	{
		return *(device->fences[index]);
	}

	void Context::CreateDeviceAndDisplay(platform::Render::DisplaySetting setting) {
		device = std::make_shared<Device>(DefaultAdapter());
		ContextEx(device->d3d_device.Get(), nullptr);

		display = std::make_shared<Display>(GetDXGIFactory4(), device->GetNodeDevice(0)->GetQueue(QueueType::Direct).D3DCommandQueue.Get(), setting, g_hwnd);//test code

		platform::Render::GCommandList.GetImmediateCommandList().SetContext(GetDefaultCommandContext());
		
		screen_frame_buffer = display->GetFrameBuffer();
		SetFrame(display->GetFrameBuffer());
	}
	void Context::DoBindFrameBuffer(const std::shared_ptr<platform::Render::FrameBuffer>& framebuffer)
	{
	}
	void Context::Render(platform::Render::CommandList& CmdList,const Effect::Effect & effect, const Effect::Technique & tech, const platform::Render::InputLayout & layout)
	{
		platform::Render::GraphicsPipelineStateInitializer GraphicsPSOInit{};

		CmdList.FillRenderTargetsInfo(GraphicsPSOInit);

		GraphicsPSOInit.ShaderPass.VertexDeclaration = static_cast<const InputLayout&>(layout).GetVertexDeclaration();

		auto index_stream = layout.GetIndexStream();

		auto vertex_count = index_stream ? layout.GetNumIndices() : layout.GetNumVertices();

		auto tt = layout.GetTopoType();
		GraphicsPSOInit.Primitive = tt;

		auto prim_count = vertex_count;
		switch (tt)
		{
		case platform::Render::PrimtivteType::LineList:
			prim_count /= 2;
			break;
		case platform::Render::PrimtivteType::TriangleList:
			prim_count /= 3;
			break;
		case platform::Render::PrimtivteType::TriangleStrip:
			prim_count -= 2;
			break;
		}

		//TODO Instance
		auto num_instances = layout.GetVertexStream(0).instance_freq;

		//Statistics Render Infomation

		auto num_passes = tech.NumPasses();

		auto BindLegacyPass = [&](ShaderCompose& compose,const platform::Render::PipleState& state)
		{
			GraphicsPSOInit.BlendState = state.BlendState;
			GraphicsPSOInit.DepthStencilState = state.DepthStencilState;
			GraphicsPSOInit.RasterizerState = state.RasterizerState;

			GraphicsPSOInit.ShaderPass.VertexShader = compose.GetVertexShader();
			GraphicsPSOInit.ShaderPass.PixelShader = compose.GetPixelShader();

			platform::Render::SetGraphicsPipelineState(CmdList, GraphicsPSOInit);

			//Vertex Stream
			auto num_vertex_streams = layout.GetVertexStreamsSize();
			for (auto i = 0; i != num_vertex_streams; ++i) {
				auto& stream = layout.GetVertexStream(i);
				auto& vb = static_cast<GraphicsBuffer&>(*stream.stream);
				CmdList.SetVertexBuffer(i, &vb);
			}

			compose.Bind();

			auto SetShaderParamters = [&]<ShaderType type,typename T>(T Shader)
			{
				if (!Shader)
					return;
				int BufferIndex = 0;
				compose.Bind<type>(CmdList);

				int SRVIndex = 0;
				for (auto& srv : compose.Srvs[type])
				{
					CmdList.SetShaderResourceView(Shader, SRVIndex, srv);
					++SRVIndex;
				}

				int SamplerIndex = 0;
				for (auto& sampler : compose.Samplers[type])
				{
					CmdList.SetShaderSampler(Shader, SamplerIndex, sampler);
					++SamplerIndex;
				}
			};
			
			SetShaderParamters.operator()<ShaderType::VertexShader>(compose.GetVertexShader());
			SetShaderParamters.operator()<ShaderType::PixelShader > (compose.GetPixelShader());

			compose.UnBind();
		};

		if (index_stream) {
			auto num_indices = layout.GetNumIndices();
			for (auto i = 0; i != num_passes; ++i) {
				auto& pass = tech.GetPass(i);

				auto& shader_compose = pass.GetShader(effect);
				auto& pipe_state = pass.GetState();
				BindLegacyPass(static_cast<ShaderCompose&>(shader_compose), pipe_state);
				CmdList.DrawIndexedPrimitive(index_stream.get(),
					layout.GetVertexStart(), 0, layout.GetNumVertices(),
					layout.GetIndexStart(), prim_count, num_instances
				);
			}
		}
		else {
			auto num_vertices = layout.GetNumVertices();
			for (auto i = 0; i != num_passes; ++i) {
				auto& pass = tech.GetPass(i);
				auto& shader_compose = pass.GetShader(effect);
				auto& pipe_state = pass.GetState();
				BindLegacyPass(static_cast<ShaderCompose&>(shader_compose), pipe_state);

				CmdList.DrawPrimitive(layout.GetVertexStart(), 0, prim_count, num_instances);
			}
		}
	}
	void Context::BeginFrame()
	{
		SetFrame(GetScreenFrame());
	}
	
	Display & Context::GetDisplay()
	{
		return *display;
	}

	RayContext& D3D12::Context::GetRayContext()
	{
		if (ray_context)
			return *ray_context;
		else
			throw white::unsupported();
	}

	CommandContext* D3D12::Context::GetDefaultCommandContext()
	{
		auto context = &(device->Devices[0]->GetDefaultCommandContext());

		return context;
	}

	Context & Context::Instance()
	{
		static Context context;
		return context;
	}
}

namespace platform_ex {
	namespace Windows {
		namespace D3D12 {
			bool Support() {
				return true;
			}
			platform::Render::Context& GetContext() {
				return Context::Instance();
			}
		}
	}
}

#if ENABLE_AFTER_MATH
void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
{
	// Create a GPU crash dump decoder object for the GPU crash dump.
	GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
		GFSDK_Aftermath_Version_API,
		pGpuCrashDump,
		gpuCrashDumpSize,
		&decoder));

	// Use the decoder object to read basic information, like application
	// name, PID, etc. from the GPU crash dump.
	GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo));


	GFSDK_Aftermath_PageFaultInformation pageInfo = {};
	GFSDK_Aftermath_GetPageFaultInformation(&pageInfo);

	// Create a unique file name for writing the crash dump data to a file.
	// Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
	// driver release) we may see redundant crash dumps. As a workaround,
	// attach a unique count to each generated file name.
	const std::string baseFileName =
		std::string("WhiteEngine")
		+ "-VA"
		+ std::to_string(pageInfo.faultingGpuVA)+"-"
		+ std::to_string(baseInfo.pid);

	// Write the the crash dump data to a file using the .nv-gpudmp extension
	// registered with Nsight Graphics.
	const std::string crashDumpFileName = baseFileName + ".nv-gpudmp";
	std::ofstream dumpFile(crashDumpFileName, std::ios::out | std::ios::binary);
	if (dumpFile)
	{
		dumpFile.write((const char*)pGpuCrashDump, gpuCrashDumpSize);
		dumpFile.close();
	}

	// Decode the crash dump to a JSON string.
	// Step 1: Generate the JSON and get the size.
	uint32_t jsonSize = 0;
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
		decoder,
		GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
		GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		&jsonSize));
	// Step 2: Allocate a buffer and fetch the generated JSON.
	std::vector<char> json(jsonSize);
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetJSON(
		decoder,
		uint32_t(json.size()),
		json.data()));

	// Write the the crash dump data as JSON to a file.
	const std::string jsonFileName = crashDumpFileName + ".json";
	std::ofstream jsonFile(jsonFileName, std::ios::out | std::ios::binary);
	if (jsonFile)
	{
		jsonFile.write(json.data(), json.size());
		jsonFile.close();

		WF_TraceRaw(platform::Descriptions::Informative, json.data());
	}

	// Destroy the GPU crash dump decoder object.
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
}
#endif
