#include "SceneInfo.h"

using namespace WhiteEngine;

inline static wm::float4x4 InvertProjectionMatrix(const wm::float4x4& ProjMatrix)
{
	if (ProjMatrix[1][0] == 0.0f &&
		ProjMatrix[3][0] == 0.0f &&
		ProjMatrix[0][1] == 0.0f &&
		ProjMatrix[3][1] == 0.0f &&
		ProjMatrix[0][2] == 0.0f &&
		ProjMatrix[1][2] == 0.0f &&
		ProjMatrix[0][3] == 0.0f &&
		ProjMatrix[1][3] == 0.0f &&
		ProjMatrix[2][3] == 1.0f &&
		ProjMatrix[3][3] == 0.0f)
	{
		// Solve the common case directly with very high precision.
		/*
		M =
		| a | 0 | 0 | 0 |
		| 0 | b | 0 | 0 |
		| s | t | c | 1 |
		| 0 | 0 | d | 0 |
		*/

		double a = ProjMatrix[0][0];
		double b = ProjMatrix[1][1];
		double c = ProjMatrix[2][2];
		double d = ProjMatrix[3][2];
		double s = ProjMatrix[2][0];
		double t = ProjMatrix[2][1];

		return wm::float4x4(
			wm::float4(static_cast<float>(1.0 / a), 0.0f, 0.0f, 0.0f),
			wm::float4(0.0f, static_cast<float>(1.0 / b), 0.0f, 0.0f),
			wm::float4(0.0f, 0.0f, 0.0f, static_cast<float>(1.0 / d)),
			wm::float4(static_cast<float>(-s / a), static_cast<float>(-t / b), 1.0f, static_cast<float>(-c / d))
		);
	}
	else
	{
		return wm::inverse(ProjMatrix);
	}
}

SceneMatrices::SceneMatrices(const Initializer& initializer)
{
	ViewMatrix = initializer.ViewMatrix;
	ViewOrigin = initializer.ViewOrigin;
	ProjectionMatrix = initializer.ProjectionMatrix;

	InvProjectionMatrix = InvertProjectionMatrix(ProjectionMatrix);
	
	ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;

	InvViewMatrix = wm::inverse(ViewMatrix);
	InvViewProjectionMatrix = InvProjectionMatrix * InvViewMatrix;
}

void SceneInfo::SetupParameters(wm::vector2<int> BufferSize, const SceneMatrices& Matrices, SceneParameters& Parameters)
{
	const auto EffectiveViewRect = ViewRect;

	// Calculate the vector used by shaders to convert clip space coordinates to texture space.
	const float InvBufferSizeX = 1.0f / BufferSize.x;
	const float InvBufferSizeY = 1.0f / BufferSize.y;
	// to bring NDC (-1..1, 1..-1) into 0..1 UV for BufferSize textures
	const wm::float4 ScreenPositionScaleBias(
		EffectiveViewRect.Width() * InvBufferSizeX / +2.0f,
		EffectiveViewRect.Height() * InvBufferSizeY / (-2.0f),
		(EffectiveViewRect.Height() / 2.0f + EffectiveViewRect.Min.y) * InvBufferSizeY,
		(EffectiveViewRect.Width() / 2.0f + EffectiveViewRect.Min.x) * InvBufferSizeX);

	Parameters.ScreenPositionScaleBias = ScreenPositionScaleBias;

	Parameters.BufferSizeAndInvSize = wm::float4(static_cast<float>(BufferSize.x), static_cast<float>(BufferSize.y),
		InvBufferSizeX, InvBufferSizeY);

	auto& ProjectionMatrix = Matrices.GetProjectionMatrix();

	Parameters.SceneDepthTexture = SceneDepth;
	Parameters.SceneDepthTextureSampler = platform::Render::TextureSampleDesc::point_sampler;
	Parameters.ScreenToWorld =wm::transpose(wm::float4x4(
		wm::float4(1,0,0,0),
		wm::float4(0,1,0,0),
		wm::float4(0,0, ProjectionMatrix[2][2],1),
		wm::float4(0, 0, ProjectionMatrix[3][2],0)
	) * Matrices.GetInvViewProjectionMatrix());
}