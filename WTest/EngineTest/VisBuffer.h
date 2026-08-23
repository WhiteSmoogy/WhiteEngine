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
#include "RenderInterface/Indirect.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "Runtime/CameraController.h"
#include "Runtime/Camera.h"
#include "Runtime/Renderer/imgui/imgui_context.h"
#include "Runtime/Renderer/Trinf.h"
#include "RenderInterface/PixelShaderUtils.h"



#include "TestFramework.h"
#include "test.h"
#include <windowsx.h>

import RenderGraph;

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
	std::shared_ptr<render::CommandSignature> draw_visidSig;
	std::shared_ptr<render::Texture2D> vis_buffer;
	std::shared_ptr<render::Texture2D> normal_buffer;
	std::shared_ptr<render::Texture2D> albedo_buffer;
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

	void RenderTrinf(RenderGraph::RGBuilder& Builder);

	white::uint32 DoUpdate(white::uint32 pass) override {
		auto& CmdList = render::CommandListExecutor::GetImmediateCommandList();

		RenderGraph::RGBuilder Builder{ CmdList,RenderGraph::RGEventName{"Frame"} };

		SCOPED_GPU_EVENT(CmdList, Frame);

		CmdList.BeginFrame();
		render::Context::Instance().BeginFrame();
		auto& screen_frame = render::Context::Instance().GetScreenFrame();
		auto screen_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::Target0));
		auto depth_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::DepthStencil));

		{
			platform::Render::RenderPassInfo ClearPass(
				screen_tex, platform::Render::RenderTargetActions::Clear_Store,
				depth_tex, platform::Render::DepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil);
			CmdList.BeginRenderPass(ClearPass, "Clear");
		}

		platform::imgui::Context_NewFrame();

		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		OnGUI();
		ImGui::Render();

		{
			SCOPED_GPU_EVENT(CmdList, Trinf);
			Trinf::Scene.AddResource(sponza_trinf);

			Trinf::Scene.BeginAsyncUpdate(Builder);

			Trinf::Scene.EndAsyncUpdate(Builder);
		}

		RenderTrinf(Builder);

		Builder.Execute();

		DrawLighting(CmdList, screen_tex, depth_tex);
		OnDrawUI(CmdList, screen_tex);
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

		platform::imgui::Context_Init(render::Context::Instance());

		auto& Device = render::Context::Instance().GetDevice();

		white::coroutine::SyncWait(platform_ex::AsyncLoadDStorageAsset("sponza_crytek/textures.dsff.asset"));
		sponza_trinf = std::static_pointer_cast<Trinf::Resources>(white::coroutine::SyncWait(platform_ex::AsyncLoadDStorageAsset("sponza_crytek/trinf.ds.asset")));

		// Crytek Sponza is Y-up and its long atrium runs along X in this asset.
		// Start inside the atrium instead of above the roof looking at the origin.
		const white::math::float3 up_vec{ 0, 1.f, 0 };
		const white::math::float3 eye{ -45.f, 6.f, -2.f };
		const white::math::float3 target{ 10.f, 9.f, 0.f };
		camera.SetViewMatrix(WhiteEngine::X::look_at_lh(eye, target, up_vec));

		// The manipulator's target distance must match the look-at point. The old
		// hard-coded value (10) made rotations orbit an unrelated point.
		const float look_at_distance = white::math::length(target - eye);
		pCameraMainpulator = std::make_unique<WhiteEngine::Core::TrackballCameraManipulator>(look_at_distance);
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

		auto& screen_frame = render::Context::Instance().GetScreenFrame();
		auto* screen_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::Target0));
		const float aspect = static_cast<float>(screen_tex->GetWidth(0)) / screen_tex->GetHeight(0);
		const float vertical_fov = 2.f * std::atan(1.f / aspect); // 90 degree horizontal FOV.
		projMatrix = WhiteEngine::X::perspective_fov_lh(vertical_fov, aspect, 0.5f, 250.f);

		//Indirect Draw CommandSig
		render::IndirectArgumentDescriptor indirctArguments[1] =
		{
			{render::IndirectArgumentType::INDIRECT_DRAW_INDEX, 0, sizeof(render::DrawIndexArguments)},
		};

		render::CommandSignatureDesc commandSigDesc =
		{
			nullptr, white::make_const_span(indirctArguments), true
		};

		draw_visidSig = white::share_raw(Device.CreateCommandSignature(commandSigDesc));

		render::ClearValueBinding invalidId;
		std::memset(invalidId.Value.Color, 0xFFFFFFFF, sizeof(invalidId.Value));

		platform::Render::ResourceCreateInfo CreateInfo{"VisBuffer"};
		CreateInfo.clear_value = &invalidId;

		const auto width = screen_tex->GetWidth(0);
		const auto height = screen_tex->GetHeight(0);
		vis_buffer = white::share_raw(Device.CreateTexture(width, height, 1, 1, EFormat::EF_R32UI, EAccessHint::SRV | EAccessHint::RTV, {}, CreateInfo));

		platform::Render::ResourceCreateInfo normalCreateInfo{ "GBuffer.Normal" };
		normalCreateInfo.clear_value = &render::ClearValueBinding::Black;
		normal_buffer = white::share_raw(Device.CreateTexture(width, height, 1, 1, EFormat::EF_ABGR8, EAccessHint::SRV | EAccessHint::RTV, {}, normalCreateInfo));

		platform::Render::ResourceCreateInfo albedoCreateInfo{ "GBuffer.Albedo" };
		albedoCreateInfo.clear_value = &render::ClearValueBinding::Black;
		albedo_buffer = white::share_raw(Device.CreateTexture(width, height, 1, 1, EFormat::EF_ABGR8, EAccessHint::SRV | EAccessHint::RTV, {}, albedoCreateInfo));
	}

	void OnGUI();

	void OnDrawUI(render::CommandList& CmdList, render::Texture* screenTex)
	{
		platform::Render::RenderPassInfo passInfo(screenTex, render::RenderTargetActions::Load_Store);

		SCOPED_GPU_EVENT(CmdList, Imgui);
		CmdList.BeginRenderPass(passInfo, "imguiPass");

		platform::imgui::Context_RenderDrawData(CmdList, ImGui::GetDrawData());
	}

	void DrawLighting(render::CommandList& CmdList, render::Texture* screenTex, render::Texture* depthTex);
};


