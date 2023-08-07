#pragma once

#define TEST_CODE 1

#include "WFramework/Helper/ShellHelper.h"

#include "Asset/DStorageAsset.h"
#include "Core/Coroutine/SyncWait.h"

#include "RenderInterface/ICommandList.h"
#include "RenderInterface/DrawEvent.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/IFrameBuffer.h"
#include "RenderInterface/ITexture.hpp"

#include "imgui/imgui_impl_win32.h"
#include "Runtime/CameraController.h"
#include "Runtime/Camera.h"
#include "Runtime/Renderer/imgui/imgui_context.h"
#include "Runtime/Renderer/Trinf.h"



#include "TestFramework.h"
#include "test.h"
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace render = platform::Render;

using render::EFormat;
using render::EAccessHint;

class VisBufferTest : public Test::TestFrameWork {
public:
	using base = Test::TestFrameWork;
	using base::base;

	WhiteEngine::Core::Camera camera;
	std::unique_ptr<WhiteEngine::Core::TrackballCameraManipulator> pCameraMainpulator;

	wm::float4x4 projMatrix;

	std::shared_ptr<Trinf::Resources> sponza_trinf;
private:
	bool SubWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
			return true;

		ImGuiIO& io = ImGui::GetIO();

		if (io.WantCaptureMouse || io.WantCaptureKeyboard)
			return true;

		return false;
	}

	void RenderTrinf(platform::Render::CommandList& CmdList);

	white::uint32 DoUpdate(white::uint32 pass) override {
		auto& CmdList = render::CommandListExecutor::GetImmediateCommandList();

		SCOPED_GPU_EVENT(CmdList, Frame);

		CmdList.BeginFrame();
		render::Context::Instance().BeginFrame();
		auto& screen_frame = render::Context::Instance().GetScreenFrame();
		auto screen_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::Target0));

		platform::imgui::Context_NewFrame();

		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::Render();

		Trinf::Scene->AddResource(sponza_trinf);

		Trinf::Scene->BeginAsyncUpdate(CmdList);

		RenderTrinf(CmdList);

		Trinf::Scene->EndAsyncUpdate(CmdList);

		CmdList.EndFrame();

		CmdList.Present(&render::Context::Instance().GetDisplay());

		render::Context::Instance().GetDisplay().WaitOnSwapBuffers();

		return Nothing;
	}

	void OnCreate() override {

		LOG_TRACE(__FUNCTION__);

		auto swap_chain = ::Create(GetNativeHandle());

		render::DisplaySetting display_setting;

		render::Context::Instance().CreateDeviceAndDisplay(display_setting);

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();


		ImGui_ImplWin32_Init(GetNativeHandle());

		platform::imgui::Context_Init(render::Context::Instance());

		Trinf::Scene = std::make_unique<Trinf::StreamingScene>();

		Trinf::Scene->Init();

		auto& Device = render::Context::Instance().GetDevice();

		white::coroutine::SyncWait(platform_ex::AsyncLoadDStorageAsset("sponza_crytek/textures.dsff.asset"));
		sponza_trinf = std::static_pointer_cast<Trinf::Resources>(white::coroutine::SyncWait(platform_ex::AsyncLoadDStorageAsset("sponza_crytek/trinf.ds.asset")));

		white::math::float3 up_vec{ 0,1.f,0 };
		white::math::float3 view_vec{ 0.94f,-0.1f,0.2f };

		white::math::float3 eye{ -20,0,0 };

		camera.SetViewMatrix(WhiteEngine::X::look_at_lh(eye, white::math::float3(0, 0, 0), up_vec));

		pCameraMainpulator = std::make_unique<WhiteEngine::Core::TrackballCameraManipulator>(10.0f);
		pCameraMainpulator->Attach(camera);
		pCameraMainpulator->SetSpeed(0.005f, 0.1f);

		GetMessageMap()[WM_MOUSEMOVE] += [&](::WPARAM wParam, ::LPARAM lParam) {
			static auto lastxPos = GET_X_LPARAM(lParam);
			static auto lastyPos = GET_Y_LPARAM(lParam);
			auto xPos = GET_X_LPARAM(lParam);
			auto yPos = GET_Y_LPARAM(lParam);
			white::math::float2 offset(static_cast<float>(xPos - lastxPos), static_cast<float>(yPos - lastyPos));
			lastxPos = xPos;
			lastyPos = yPos;
			if (wParam & MK_LBUTTON) {
				pCameraMainpulator->Rotate(offset);
			}
			if (wParam & MK_MBUTTON) {
				pCameraMainpulator->Move(offset);
			}
			if (wParam & MK_RBUTTON) {
				pCameraMainpulator->Zoom(offset);
			}
			};

		float aspect = 1280;
		aspect /= 720;

		float fov = atan(1 / aspect);

		projMatrix = WhiteEngine::X::perspective_fov_lh(fov * 2, aspect, 1, 1000);
	}
};


