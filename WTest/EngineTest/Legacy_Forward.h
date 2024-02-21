#pragma once

#define TEST_CODE 1

#include "TestFramework.h"
#include "Entities.h"
#include "Runtime/CameraController.h"
#include "Runtime/Camera.h"
#include "Runtime/Renderer/DirectionalLight.h"
#include "RenderInterface/DataStructures.h"
#include "RenderInterface/ITexture.hpp"
#include "Runtime/Renderer/PostProcess/PostProcessCombineLUTs.h"
#include "Runtime/Renderer/PostProcess/PostProcessToneMap.h"
#include "Runtime/Renderer/ShadowRendering.h"
#include "RenderInterface/DrawEvent.h"
#include "RenderInterface/ICommandList.h"
#include "Runtime/Renderer/imgui/imgui_context.h"
#include "Runtime/ParallelFor.h"
#include "RenderInterface/PipelineStateUtility.h"
#include "Runtime/Renderer/ScreenSpaceDenoiser.h"
#include "Runtime/Renderer/GizmosElements.h"
#include "Core/Math/RotationMatrix.h"

#include "imgui/backends/imgui_impl_win32.h"
#include "Core/Math/TranslationMatrix.h"

#include "WFramework/Helper/ShellHelper.h"
#include "Asset/DStorageAsset.h"
#include "Core/Coroutine/SyncWait.h"

#include "test.h"
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace we = WhiteEngine;
namespace render = platform::Render;

using render::EFormat;
using render::EAccessHint;

class ForwardTest : public Test::TestFrameWork {
public:
	using base = Test::TestFrameWork;
	using base::base;

	std::unique_ptr<Entities> pEntities;
	WhiteEngine::Core::Camera camera;
	std::unique_ptr<WhiteEngine::Core::TrackballCameraManipulator> pCameraMainpulator;

	static_assert(sizeof(we::Render::DirectLight) == sizeof(wm::float4) + sizeof(wm::float4) + sizeof(wm::float4) + 4);

	std::vector<we::Render::DirectLight> lights;
	std::shared_ptr<render::GraphicsBuffer> pLightConstantBuffer;

	WhiteEngine::DirectionalLight sun_light;
	we::SceneInfo scene;


	std::shared_ptr<render::Texture2D> RayShadowMask;
	std::shared_ptr<render::UnorderedAccessView> RayShadowMaskUAV;

	std::shared_ptr<render::Texture2D> RayShadowMaskDenoiser;
	std::shared_ptr<render::UnorderedAccessView> RayShadowMaskDenoiserUAV;

	std::shared_ptr<render::Texture2D> ShadowMap;
	std::shared_ptr<render::Texture2D> ShadowMapMask;


	std::shared_ptr<render::Texture2D> HDROutput;
	std::shared_ptr<render::Texture2D> NormalOutput;


	white::math::float4 clear_color = { 0,0,0,0 };

	platform::CombineLUTSettings lut_params;
	bool lut_dirty = false;
	render::TexturePtr lut_texture = nullptr;

	float LightHalfAngle = 0.5f;
	int SamplesPerPixel = 1;
	unsigned StateFrameIndex = 0;

	std::vector<std::unique_ptr<we::ProjectedShadowInfo>> ShadowInfos;

	WhiteEngine::Sphere FreezeSphere[10];
	int FreezeCount = 0;
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

	void OnGUI()
	{
		ImGui::Begin("Settings");
		OnRaytracingUI();
		OnCombineLUTUI();
		OnGizmosUI();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	white::uint32 DoUpdate(white::uint32 pass) override {
		auto& timer = platform::chrono::FetchGlobalTimer();
		timer.UpdateOnFrameStart();
		platform::Material::GetInstanceEvaluator().Define("time", timer.GetFrameTime(), true);

		auto& CmdList = platform::Render::CommandListExecutor::GetImmediateCommandList();

		SCOPED_GPU_EVENT(CmdList, Frame);


		CmdList.BeginFrame();
		render::Context::Instance().BeginFrame();
		auto& screen_frame = render::Context::Instance().GetScreenFrame();
		auto screen_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::Target0));
		auto depth_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::DepthStencil));

		auto& Device = render::Context::Instance().GetDevice();

		platform::imgui::Context_NewFrame();

		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		OnGUI();
		ImGui::Render();

		wm::float4x4 worldmatrix = {
			{1,0,0,0},
			{0,1,0,0},
			{0,0,1,0},
			{0,0,0,1}
		};
		float aspect = screen_tex->GetWidth(0);
		aspect /= screen_tex->GetHeight(0);

		float fov = atan(1 / aspect);

		auto projmatrix = WhiteEngine::X::perspective_fov_lh(fov * 2, aspect, 1, 1000);
		auto viewmatrix = camera.GetViewMatrix();

		auto viewproj = viewmatrix * projmatrix;

		//SetupViewFrustum
		scene.NearClippingDistance = -projmatrix[3][2];

		we::SceneMatrices::Initializer matrices;
		matrices.ViewMatrix = viewmatrix;
		matrices.ProjectionMatrix = projmatrix;
		matrices.ViewOrigin = camera.GetEyePos();

		scene.Matrices = we::SceneMatrices{ matrices };
		scene.BufferSize.x = screen_tex->GetWidth(0);
		scene.BufferSize.y = screen_tex->GetHeight(0);
		scene.ViewRect = we::IntRect(0, 0, scene.BufferSize.x, scene.BufferSize.y);
		scene.SceneDepth = depth_tex;

		scene.SetupParameters();

		auto pEffect = platform::X::LoadEffect("ForwardDirectLightShading");
		auto pPreZEffect = platform::X::LoadEffect("PreZ");

		using namespace std::literals;

		//viewinfo materialinfo
		{
			//obj
			pEffect->GetParameter("world"sv) = wm::transpose(worldmatrix);
			//camera
			pEffect->GetParameter("camera_pos"sv) = camera.GetEyePos();
			pEffect->GetParameter("viewproj"sv) = wm::transpose(viewproj);

			//light
			pEffect->GetParameter("light_count"sv) = static_cast<white::uint32>(lights.size());
			pEffect->GetParameter("inv_sscreen"sv) = wm::float2(1 / 1280.f, 1 / 720.f);

			pEffect->GetParameter("lights") = pLightConstantBuffer;

			//mat
			pEffect->GetParameter("alpha"sv) = 1.0f;

			//light_ext
			pEffect->GetParameter("ambient_color") = white::math::float3(0.1f, 0.1f, 0.1f);

			{
				//obj
				pPreZEffect->GetParameter("world"sv) = wm::transpose(worldmatrix);
				//camera
				pPreZEffect->GetParameter("camera_pos"sv) = camera.GetEyePos();
				pPreZEffect->GetParameter("viewproj"sv) = wm::transpose(viewproj);
			}
		}

		{
			SCOPED_GPU_EVENT(CmdList, PreZ);
			platform::Render::RenderPassInfo prezPass(
				NormalOutput.get(), platform::Render::RenderTargetActions::Clear_Store,
				depth_tex, platform::Render::DepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil);
			CmdList.BeginRenderPass(prezPass, "PreZ");

			//pre-z
			for (auto& entity : pEntities->GetRenderables())
			{
				//copy normal map params
				entity.GetMaterial().UpdateParams(reinterpret_cast<const platform::Renderable*>(&entity), pPreZEffect.get());

				render::Context::Instance().Render(CmdList, *pPreZEffect, pPreZEffect->GetTechniqueByIndex(0), entity.GetMesh().GetInputLayout());
			}
		}

		OnDrawLights(CmdList, viewmatrix, projmatrix);
		RenderShadowDepth();

		if (RayShadowMaskDenoiser)
			pEffect->GetParameter("rayshadow_tex") = render::TextureSubresource(RayShadowMaskDenoiser, 0, RayShadowMask->GetArraySize(), 0, RayShadowMaskDenoiser->GetNumMipMaps());
		if (ShadowMap) {
			pEffect->GetParameter("shadowmask_tex") = render::TextureSubresource(ShadowMapMask, 0, ShadowMapMask->GetArraySize(), 0, ShadowMapMask->GetNumMipMaps());
		}

		{
			SCOPED_GPU_EVENT(CmdList, GeometryShading);
			platform::Render::RenderPassInfo GeometryPass(
				HDROutput.get(), platform::Render::RenderTargetActions::Clear_Store,
				depth_tex, platform::Render::DepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil);
			CmdList.BeginRenderPass(GeometryPass, "Geometry");

			for (auto& entity : pEntities->GetRenderables())
			{
				entity.GetMaterial().UpdateParams(reinterpret_cast<const platform::Renderable*>(&entity));
				render::Context::Instance().Render(CmdList, *pEffect, pEffect->GetTechniqueByIndex(0), entity.GetMesh().GetInputLayout());
			}
		}

		OnPostProcess(CmdList, screen_tex);
		OnDrawGizmos(depth_tex, CmdList);
		OnDrawUI(CmdList);

		CmdList.EndFrame();

		CmdList.Present(&render::Context::Instance().GetDisplay());

		render::Context::Instance().GetDisplay().WaitOnSwapBuffers();

		++StateFrameIndex;

		return Nothing;
	}

	render::Texture* GetScreenTex()
	{
		return render::Context::Instance().GetScreenFrame()->Attached(render::FrameBuffer::Target0);
	}

	float Square(float v)
	{
		return v * v;
	}

	bool SphereAABBIntersection(const wm::float3& SphereCenter, const float RadiusSquared, const wm::float3& Min, const wm::float3& Max)
	{
		// Accumulates the distance as we iterate axis
		float DistSquared = 0.f;
		// Check each axis for min/max and add the distance accordingly
		// NOTE: Loop manually unrolled for > 2x speed up
		if (SphereCenter.x < Min.x)
		{
			DistSquared += Square(SphereCenter.x - Min.x);
		}
		else if (SphereCenter.x > Max.x)
		{
			DistSquared += Square(SphereCenter.x - Max.x);
		}
		if (SphereCenter.y < Min.y)
		{
			DistSquared += Square(SphereCenter.y - Min.y);
		}
		else if (SphereCenter.y > Max.y)
		{
			DistSquared += Square(SphereCenter.y - Max.y);
		}
		if (SphereCenter.z < Min.z)
		{
			DistSquared += Square(SphereCenter.z - Min.z);
		}
		else if (SphereCenter.z > Max.z)
		{
			DistSquared += Square(SphereCenter.z - Max.z);
		}
		// If the distance is less than or equal to the radius, they intersect
		return DistSquared <= RadiusSquared;
	}

	void RenderShadowDepth()
	{
		ShadowInfos.clear();

		//ViewDependentWholeSceneShadowsForView
		auto ProjectCount = static_cast<int32>(sun_light.GetNumViewDependentWholeSceneShadows(scene));

		std::array<wm::float4x4, 8> ProjectionMatrix;
		for (int32 Index = 0; Index < ProjectCount; ++Index)
		{
			const uint32 ShadowBorder = 4;

			const uint32 MaxShadowResolution = 2048;

			we::WholeSceneProjectedShadowInitializer initializer;
			sun_light.GetProjectedShadowInitializer(scene, Index, initializer);

			auto& ProjectedShadowInfo = ShadowInfos.emplace_back(std::make_unique<we::ProjectedShadowInfo>());
			ProjectedShadowInfo->SetupWholeSceneProjection(
				scene,
				initializer,
				MaxShadowResolution - ShadowBorder * 2,
				MaxShadowResolution - ShadowBorder * 2,
				ShadowBorder);

			ProjectionMatrix[std::min(Index, 7)] = wm::transpose(we::TranslationMatrix(initializer.ShadowTranslation) * ProjectedShadowInfo->SubjectAndReceiverMatrix);
		}

		//AllocateCSMDepthTargets
		bool Atlasing = true;

		//TODO TextureLayout
		uint32 LayoutX = 0;
		uint32 LayoutY = 0;
		uint32 LayoutWidth = 0;
		uint32 LayoutHeight = 0;

		for (int32 ShadowIndex = 0; ShadowIndex < ShadowInfos.size(); ++ShadowIndex)
		{
			auto ElementSizeX = ShadowInfos[ShadowIndex]->ResolutionX + 2 * ShadowInfos[ShadowIndex]->BorderSize;

			LayoutWidth += ShadowInfos[ShadowIndex]->ResolutionX + 2 * ShadowInfos[ShadowIndex]->BorderSize;
			LayoutHeight = std::max(LayoutHeight, ShadowInfos[ShadowIndex]->ResolutionY + 2 * ShadowInfos[ShadowIndex]->BorderSize);

			ShadowInfos[ShadowIndex]->X = LayoutX;
			ShadowInfos[ShadowIndex]->Y = LayoutY;

			LayoutX += ElementSizeX;
		}

		auto& Device = render::Context::Instance().GetDevice();
		if (!ShadowMap || ShadowMap->GetWidth(0) < LayoutWidth || ShadowMap->GetHeight(0) < LayoutHeight)
			ShadowMap = white::share_raw(Device.CreateTexture(LayoutWidth, LayoutHeight, 1, 1, render::EFormat::EF_D32F, EAccessHint::SRV | EAccessHint::DSV, {}));

		//Frustum Cull
		std::vector<std::vector<const Entity*>> Subjects{ ShadowInfos.size() };

		{
			std::vector<white::coroutine::Task<>> taskes;

			WhiteEngine::ParallelFor(static_cast<int32>(ShadowInfos.size()),
				[this, &Subjects](int ShadowIndex) {
					for (int32 PrimitiveIndex = 0; PrimitiveIndex < pEntities->GetRenderables().size(); ++PrimitiveIndex)
					{
						auto& Primitive = pEntities->GetRenderables()[PrimitiveIndex];

						we::Sphere ShadowBounds = ShadowInfos[ShadowIndex]->ShadowBounds;

						we::BoxSphereBounds PrimitiveBounds;
						PrimitiveBounds.Origin = (Primitive.GetMesh().GetBoundingMin() + Primitive.GetMesh().GetBoundingMax()) / 2;
						PrimitiveBounds.Extent = (Primitive.GetMesh().GetBoundingMax() - Primitive.GetMesh().GetBoundingMin()) / 2;
						PrimitiveBounds.Radius = wm::length(PrimitiveBounds.Extent);

						const auto LightDirection = sun_light.GetDirection();
						const auto PrimitiveToShadowCenter = ShadowBounds.Center - PrimitiveBounds.Origin;
						const auto ProjectedDistanceFromShadowOriginAlongLightDir = wm::dot(PrimitiveToShadowCenter, LightDirection);
						const auto PrimitiveDistanceFromCylinderAxisSq = wm::length_sq(LightDirection * (-ProjectedDistanceFromShadowOriginAlongLightDir) + PrimitiveToShadowCenter);
						const auto CombinedRadiusSq = Square(ShadowBounds.W + PrimitiveBounds.Radius);
						if (PrimitiveDistanceFromCylinderAxisSq < CombinedRadiusSq)
						{
							Subjects[ShadowIndex].emplace_back(&Primitive);
						}
					}
				});
		}

		//RenderShadowDepthMapAtlases
		auto BeginShadowRenderPass = [this](platform::Render::CommandList& CmdList, bool Clear)
			{
				platform::Render::RenderTargetLoadAction DepthLoadAction = Clear ? render::RenderTargetLoadAction::Clear : render::RenderTargetLoadAction::Load;

				platform::Render::RenderPassInfo RPInfo(ShadowMap.get(), render::MakeDepthStencilTargetActions(MakeRenderTargetActions(DepthLoadAction, render::RenderTargetStoreAction::Store),
					render::RenderTargetActions::DontLoad_DontStore));

				CmdList.BeginRenderPass(RPInfo, "ShadowDepth Atlas");
			};

		auto& CmdList = platform::Render::CommandListExecutor::GetImmediateCommandList();
		SCOPED_GPU_EVENTF(CmdList, "ShadowDepth Atlas %dx%d", LayoutWidth, LayoutHeight);
		BeginShadowRenderPass(CmdList, true);
		for (int32 ShadowIndex = 0; ShadowIndex < ShadowInfos.size(); ++ShadowIndex)
		{
			SCOPED_GPU_EVENTF(CmdList, "ShadowIndex%d", ShadowIndex);

			BeginShadowRenderPass(CmdList, false);
			auto psoInit = ShadowInfos[ShadowIndex]->SetupShadowDepthPass(CmdList, ShadowMap.get());

			for (auto pEntity : Subjects[ShadowIndex])
			{
				auto& layout = pEntity->GetMesh().GetInputLayout();

				DrawInputLayout(CmdList, psoInit, layout);
			}
		}

		SCOPED_GPU_EVENTF(CmdList, "ShadowProjections %d", ShadowInfos.size());
		platform::Render::RenderPassInfo RPInfo(ShadowMapMask.get(), render::MakeRenderTargetActions(render::RenderTargetLoadAction::Load,
			render::RenderTargetStoreAction::Store));
		CmdList.BeginRenderPass(RPInfo, "ShadowProjections");

		for (int32 ShadowIndex = static_cast<int32>(ShadowInfos.size()) - 1; ShadowIndex >= 0; --ShadowIndex)
		{
			ShadowInfos[ShadowIndex]->RenderProjection(CmdList, scene);
		}

		//const auto ProjectionMatrix = we::TranslationMatrix(initializer.ShadowTranslation) * ProjectedShadowInfo.SubjectAndReceiverMatrix;

		auto pEffect = platform::X::LoadEffect("ForwardDirectLightShading");

		pEffect->GetParameter("ReceiverMatrix") = ProjectionMatrix;
	}

	void DrawInputLayout(platform::Render::CommandList& CmdList, platform::Render::GraphicsPipelineStateInitializer psoInit, const platform::Render::InputLayout& layout)
	{
		psoInit.ShaderPass.VertexDeclaration = layout.GetVertexDeclaration();
		psoInit.Primitive = layout.GetTopoType();

		platform::Render::SetGraphicsPipelineState(CmdList, psoInit);

		auto num_vertex_streams = layout.GetVertexStreamsSize();
		for (auto i = 0; i != num_vertex_streams; ++i) {
			auto& stream = layout.GetVertexStream(i);
			auto& vb = static_cast<render::GraphicsBuffer&>(*stream.stream);
			CmdList.SetVertexBuffer(i, &vb);
		}

		auto index_stream = layout.GetIndexStream();
		auto vertex_count = index_stream ? layout.GetNumIndices() : layout.GetNumVertices();
		auto prim_count = vertex_count;
		switch (psoInit.Primitive)
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

		auto num_instances = layout.GetVertexStream(0).instance_freq;
		if (index_stream) {
			auto num_indices = layout.GetNumIndices();
			CmdList.DrawIndexedPrimitive(index_stream.get(),
				layout.GetVertexStart(), 0, layout.GetNumVertices(),
				layout.GetIndexStart(), prim_count, num_instances
			);
		}
		else {
			auto num_vertices = layout.GetNumVertices();
			CmdList.DrawPrimitive(layout.GetVertexStart(), 0, prim_count, num_instances);
		}
	}

	void OnPostProcess(render::CommandList& CmdList, render::Texture2D* display)
	{
		SCOPED_GPU_EVENT(CmdList, PostProcess);

		//PostProcess
		if (lut_dirty || !lut_texture)
		{
			SCOPED_GPU_EVENT(CmdList, CombineLUTPass);
			lut_texture = platform::CombineLUTPass(CmdList, lut_params);
		}

		platform::TonemapInputs tonemap_inputs;
		tonemap_inputs.OverrideOutput.Texture = display;
		tonemap_inputs.OverrideOutput.LoadAction = render::RenderTargetLoadAction::NoAction;
		tonemap_inputs.OverrideOutput.StoreAction = render::RenderTargetStoreAction::Store;
		tonemap_inputs.ColorGradingTexture = lut_texture;
		tonemap_inputs.SceneColor = HDROutput;

		platform::TonemapPass(CmdList, tonemap_inputs);
	}

	void OnDrawLights(render::CommandList& CmdList, const white::math::float4x4& viewmatrix, const white::math::float4x4& projmatrix)
	{
		SCOPED_GPU_EVENT(CmdList, DrawLights);

		//TODO: don't support create resource at other thread when cmd execute
		//D3D12 CORRUPTION: Two threads were found to be executing methods associated with the same CommandList at the same time. 
		auto& screen_frame = render::Context::Instance().GetScreenFrame();
		auto screen_tex = screen_frame->Attached(render::FrameBuffer::Target0);
		auto depth_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::DepthStencil));

		auto width = depth_tex->GetWidth(0);
		auto height = depth_tex->GetHeight(0);

		auto viewproj = viewmatrix * projmatrix;

		auto invview = white::math::inverse(viewmatrix);


		auto ViewSizeAndInvSize = white::math::float4(width, height, 1.0f / width, 1.0f / height);
		// setup a matrix to transform float4(SvPosition.xyz,1) directly to World (quality, performance as we don't need to convert or use interpolator)

		float Mx = 2.0f * ViewSizeAndInvSize.z;
		float My = -2.0f * ViewSizeAndInvSize.w;
		float Ax = -1.0f;
		float Ay = 1.0f;

		white::math::float4x4 SVPositionToWorld = white::math::float4x4(
			white::math::float4(Mx, 0, 0, 0),
			white::math::float4(0, My, 0, 0),
			white::math::float4(0, 0, 1, 0),
			white::math::float4(Ax, Ay, 0, 1)
		) * scene.Matrices.GetInvViewProjectionMatrix();

		platform::Render::ShadowRGParameters shadowconstant{
			.SVPositionToWorld = white::math::transpose(SVPositionToWorld),
			.WorldCameraOrigin = camera.GetEyePos(),
			.BufferSizeAndInvSize = white::math::float4(width,height,1.0f / width,1.0f / height),
			.NormalBias = 0.1f,
			.LightDirection = lights[0].direction,
			.SourceRadius = sinf(LightHalfAngle * 3.14159265f / 180),
			.SamplesPerPixel = static_cast<unsigned>(SamplesPerPixel),
			.StateFrameIndex = StateFrameIndex,
			.WorldNormalBuffer = NormalOutput.get(),
			.Depth = depth_tex,
			.Output = RayShadowMaskUAV.get()
		};

		auto Scene = pEntities->CreateRayTracingScene();
		CmdList.BuildAccelerationStructure(Scene);

		CmdList.InsertCommand([=](render::CommandListBase& CmdList) {
			render::Context::Instance().GetRayContext().RayTraceShadow(Scene.get(), shadowconstant);
			});

		platform::ScreenSpaceDenoiser::ShadowVisibilityInput svinput =
		{
			.Mask = RayShadowMask.get(),
			.SceneDepth = depth_tex,
			.WorldNormal = NormalOutput.get(),
			.LightHalfRadians = LightHalfAngle * 3.14159265f / 180,
		};

		platform::ScreenSpaceDenoiser::ShadowVisibilityOutput svoutput =
		{
			.Mask = RayShadowMaskDenoiser.get(),
			.MaskUAV = RayShadowMaskDenoiserUAV.get()
		};

		platform::ScreenSpaceDenoiser::ShadowViewInfo viewinfo =
		{
			.StateFrameIndex = StateFrameIndex,
			.ScreenToTranslatedWorld = white::math::float4x4(
			white::math::float4(1, 0, 0, 0),
			white::math::float4(0, 1, 0, 0),
			white::math::float4(0, 0, projmatrix[2][2], 1),
			white::math::float4(0, 0, projmatrix[3][2], 0)
			) * scene.Matrices.GetInvViewProjectionMatrix(),
			.ViewToClip = projmatrix,
			.TranslatedWorldToView = viewmatrix,
			.InvProjectionMatrix = scene.Matrices.GetInvProjectionMatrix(),
			.InvDeviceZToWorldZTransform = WhiteEngine::CreateInvDeviceZToWorldZTransform(projmatrix),
		};

		{
			SCOPED_GPU_EVENT(CmdList, ShadowDenoise);

			platform::ScreenSpaceDenoiser::DenoiseShadowVisibilityMasks(
				CmdList,
				viewinfo,
				svinput,
				svoutput
			);
		}
	}

	void OnDrawGizmos(render::Texture2D* depth_tex, render::CommandList& CmdList)
	{
		we::GizmosElements gizmos;

		we::LinearColor Colors[] =
		{
			wm::float4{1,0,0,1},//Read
			wm::float4{1,1,0,1},//Yellow
			wm::float4{0,1,0,1},//Green
			wm::float4{0,0,1,1},//Blue
		};

		int Count = std::min<int>(4, FreezeCount);
		for (int i = 0; i != Count; ++i)
		{
			auto sphere = FreezeSphere[i];
			gizmos.AddSphere(sphere.Center, sphere.W, Colors[i]);
		}

		gizmos.AddAABB(pEntities->min, pEntities->max, Colors[1]);

		render::RenderPassInfo GeometryPass(
			GetScreenTex(), render::RenderTargetActions::Load_Store,
			depth_tex, render::DepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil);
		CmdList.BeginRenderPass(GeometryPass, "Gizmos");

		gizmos.Draw(CmdList, scene);
	}

	void OnDrawUI(render::CommandList& CmdList)
	{
		platform::Render::RenderPassInfo passInfo(GetScreenTex(), render::RenderTargetActions::Load_Store);

		SCOPED_GPU_EVENT(CmdList, Imgui);
		CmdList.BeginRenderPass(passInfo, "imguiPass");

		platform::imgui::Context_RenderDrawData(CmdList, ImGui::GetDrawData());
	}

	void OnCreate() override {
		LOG_TRACE(__FUNCTION__);

		auto swap_chain = ::Create(GetNativeHandle());

		render::DisplaySetting display_setting;

		render::Context::Instance().CreateDeviceAndDisplay(display_setting);

		platform::imgui::Context_Init(render::Context::Instance());

		auto& Device = render::Context::Instance().GetDevice();

		render::ResourceCreateInfo data{"HDROutput"};
		data.clear_value = &render::ClearValueBinding::Black;

		HDROutput = white::share_raw(Device.CreateTexture(1280, 720, 1, 1, render::EFormat::EF_ABGR16F, EAccessHint::SRV | EAccessHint::RTV, {}, data));
		data.Name = "NormalOutput";
		NormalOutput = white::share_raw(Device.CreateTexture(1280, 720, 1, 1, render::EFormat::EF_ABGR16F, EAccessHint::SRV | EAccessHint::RTV, {}, data));

		//sync load
		white::coroutine::SyncWait(platform_ex::AsyncLoadDStorageAsset("sponza_crytek/textures.dsff.asset"));
		pEntities = std::make_unique<Entities>("sponza_crytek.entities.wsl");

		scene.AABBMin = pEntities->min;
		scene.AABBMax = pEntities->max;

		float modelRaidus = white::math::length(pEntities->max - pEntities->min) * .5f;

		auto  eye = (pEntities->max + pEntities->min) * .5f;

		eye = eye + white::math::float3(modelRaidus * .51f, 1.2f, 0.f);

		white::math::float3 up_vec{ 0,1.f,0 };
		white::math::float3 view_vec{ 0.94f,-0.1f,0.2f };

		camera.SetViewMatrix(WhiteEngine::X::look_at_lh(eye, white::math::float3(0, 0, 0), up_vec));

		pCameraMainpulator = std::make_unique<WhiteEngine::Core::TrackballCameraManipulator>(10.0f);
		pCameraMainpulator->Attach(camera);
		pCameraMainpulator->SetSpeed(0.005f, 0.1f);

		/*DirectLight point_light;
		point_light.type = POINT_LIGHT;
		point_light.position = wm::float3(0, 40, 0);
		point_light.range = 80;
		point_light.blub_innerangle = 40;
		point_light.color = wm::float3(1.0f, 1.0f, 1.0f);*/
		//lights.push_back(point_light);

		we::Render::DirectLight directioal_light;
		directioal_light.type = we::Render::DIRECTIONAL_LIGHT;
		directioal_light.direction = wm::float3(0, 1, 0);
		directioal_light.color = wm::float3(1.0f, 1.0f, 1.0f);
		lights.push_back(directioal_light);

		sun_light.SetTransform(we::RotationMatrix(we::Rotator(-directioal_light.direction)));

		sun_light.DynamicShadowCascades = 1;
		sun_light.WholeSceneDynamicShadowRadius = 60;

		render::ResourceCreateInfoEx lightCreate{ lights.data(),"lights"};

		pLightConstantBuffer = white::share_raw(Device.CreateBuffer(render::Buffer::Usage::Dynamic, EAccessHint::SRV | EAccessHint::Structured, sizeof(lights[0]) * lights.size(), static_cast<EFormat>(sizeof(lights[0])), lightCreate));

		RayShadowMask = white::share_raw(Device.CreateTexture(1280, 720, 1, 1, EFormat::EF_ABGR16F, EAccessHint::SRV | EAccessHint::UAV, {}));
		RayShadowMaskDenoiser = white::share_raw(Device.CreateTexture(1280, 720, 1, 1, EFormat::EF_R32F, EAccessHint::SRV | EAccessHint::UAV, {}));

		ShadowMapMask = white::share_raw(Device.CreateTexture(1280, 720, 1, 1, EFormat::EF_ABGR16F, EAccessHint::SRV | EAccessHint::RTV, {}));

		RayShadowMaskUAV = white::share_raw(Device.CreateUnorderedAccessView(RayShadowMask.get()));
		RayShadowMaskDenoiserUAV = white::share_raw(Device.CreateUnorderedAccessView(RayShadowMaskDenoiser.get()));

		ShadowMap = white::share_raw(Device.CreateTexture(4096, 4096, 1, 1, EFormat::EF_D32F, EAccessHint::SRV | EAccessHint::DSV, {}));

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
	}

	void OnCombineLUTUI()
	{
		if (ImGui::CollapsingHeader("PostProcess"))
		{
#define FLOAT4_FIELD(Field) lut_dirty|=ImGui::ColorEdit4(#Field, lut_params.Field.data)
#define FLOAT_FIELD(Field) lut_dirty|=ImGui::SliderFloat(#Field, &lut_params.Field,0,1)
			if (ImGui::CollapsingHeader("ColorCorrect"))
			{
				ImGui::Indent(16);
				FLOAT4_FIELD(ColorSaturation);
				FLOAT4_FIELD(ColorContrast);
				FLOAT4_FIELD(ColorGamma);
				FLOAT4_FIELD(ColorGain);
				FLOAT4_FIELD(ColorOffset);
				FLOAT4_FIELD(ColorSaturationShadows);
				FLOAT4_FIELD(ColorContrastShadows);
				FLOAT4_FIELD(ColorGammaShadows);
				FLOAT4_FIELD(ColorGainShadows);
				FLOAT4_FIELD(ColorOffsetShadows);
				FLOAT4_FIELD(ColorSaturationMidtones);
				FLOAT4_FIELD(ColorContrastMidtones);
				FLOAT4_FIELD(ColorGammaMidtones);
				FLOAT4_FIELD(ColorGainMidtones);
				FLOAT4_FIELD(ColorOffsetMidtones);
				FLOAT4_FIELD(ColorSaturationHighlights);
				FLOAT4_FIELD(ColorContrastHighlights);
				FLOAT4_FIELD(ColorGammaHighlights);
				FLOAT4_FIELD(ColorGainHighlights);
				FLOAT4_FIELD(ColorOffsetHighlights);
				FLOAT_FIELD(ColorCorrectionShadowsMax);
				FLOAT_FIELD(ColorCorrectionHighlightsMin);
				ImGui::Unindent(16);
			}

			if (ImGui::CollapsingHeader("WhiteBlance"))
			{
				ImGui::Indent(16);
				lut_dirty |= ImGui::SliderFloat("WhiteTemp", &lut_params.WhiteTemp, 2000, 15000);
				lut_dirty |= ImGui::SliderFloat("WhiteTint", &lut_params.WhiteTint, -1, 1);
				ImGui::Unindent(16);
			}

			if (ImGui::CollapsingHeader("ACES"))
			{
				ImGui::Indent(16);
				FLOAT_FIELD(FilmSlope);
				FLOAT_FIELD(FilmToe);
				FLOAT_FIELD(FilmShoulder);
				FLOAT_FIELD(FilmBlackClip);
				FLOAT_FIELD(FilmWhiteClip);
				ImGui::Unindent(16);
			}

			lut_dirty |= ImGui::SliderFloat("Gamma", &Environment->Gamma, 1, 2.5f);
			FLOAT_FIELD(BlueCorrection);
			FLOAT_FIELD(ExpandGamut);
#undef FLOAT4_FIELD
#undef FLOAT_FIELD
		}

		if (ImGui::CollapsingHeader("Shadow"))
		{
			ImGui::Indent(16);
			ImGui::SliderFloat("ShadowRadius", &sun_light.WholeSceneDynamicShadowRadius, 0, 200);
			ImGui::SliderInt("ShadowCascades", &sun_light.DynamicShadowCascades, 1, scene.MaxShadowCascades);
			ImGui::SliderFloat("CascadeDistributionExponent", &sun_light.CascadeDistributionExponent, 2, 4);

			if (ImGui::Button("Freeze"))
			{
				FreezeCount = 0;
				for (int i = 0; i != ShadowInfos.size(); ++i)
				{
					FreezeSphere[FreezeCount++] = ShadowInfos[i]->ShadowBounds;
				}
			}
			ImGui::Unindent(16);
		}
	}

	void OnRaytracingUI()
	{
		if (ImGui::CollapsingHeader("Raytracing"))
		{
			ImGui::Indent(16);

			ImGui::SliderFloat("SoftShadowAngle", &LightHalfAngle, 0.1f, 10);
			ImGui::SliderInt("RayPerPixel", &SamplesPerPixel, 1, 8);

			ImGui::Unindent(16);
		}
	}

	void OnGizmosUI()
	{
		auto label = [&](int index, wm::float3 v)
			{
				ImGui::Text("%d\tCenter:%.3f,%.3f,%.3f", index, (float)v.x, (float)v.y, (float)v.z);
			};

		auto sphere = [&](int index, we::Sphere s)
			{
				label(index, s.Center);
				ImGui::SameLine();
				ImGui::Text(" R:%.3f", s.W);
			};

		if (ImGui::CollapsingHeader("Gizmos"))
		{
			ImGui::Indent(16);


			for (int i = 0; i != FreezeCount; ++i)
			{
				sphere(i, FreezeSphere[i]);
			}

			ImGui::Unindent(16);
		}
	}
};

