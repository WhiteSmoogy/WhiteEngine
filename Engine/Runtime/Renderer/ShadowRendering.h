#pragma once

#include <WBase/wmath.hpp>
#include "RenderInterface/ICommandList.h"
#include "Core/Math/IntRect.h"
#include "Core/Math/Sphere.h"

#include "SceneInfo.h"
#include "SceneClasses.h"

namespace WhiteEngine
{
	namespace wr = platform::Render;
	namespace wm = white::math;

	class ProjectedShadowInfo
	{
	public:
		wm::float3 PreShadowTranslation;

		wm::float4x4 ShadowViewMatrix;

		wm::float4x4 SubjectAndReceiverMatrix;

		float InvMaxSubjectDepth = 0;

		wr::Texture2D* DepthTarget;

		float MaxSubjectZ = 0;
		float MinSubjectZ = 0;

		Sphere ShadowBounds;

		ShadowCascadeSettings CascadeSettings;


		/**
		* X and Y position of the shadow in the appropriate depth buffer.  These are only initialized after the shadow has been allocated.
		 * The actual contents of the shadowmap are at X + BorderSize, Y + BorderSize.
		*/
		uint32 X = 0;
		uint32 Y = 0;

		/**
		 * Resolution of the shadow, excluding the border.
		 * The full size of the region allocated to this shadow is therefore ResolutionX + 2 * BorderSize, ResolutionY + 2 * BorderSize.
		 */
		uint32 ResolutionX = 0;
		uint32 ResolutionY = 0;

		/** Size of the border, if any, used to allow filtering without clamping for shadows stored in an atlas. */
		uint32 BorderSize = 0;

		void SetupWholeSceneProjection(const SceneInfo& scne, const WholeSceneProjectedShadowInitializer& ShadowInfo, uint32 InResolutionX, uint32 InResoultionY, uint32 InBorderSize);

		wr::GraphicsPipelineStateInitializer SetupShadowDepthPass(wr::CommandList& CmdList, wr::Texture2D* Target);

		void RenderProjection(wr::CommandList& CmdList, const SceneInfo& scene);

		wm::float4x4 GetScreenToShadowMatrix(const SceneInfo& scene) const
		{
			return GetScreenToShadowMatrix(scene, X, Y, ResolutionX, ResolutionY);
		}

		wm::float4x4 GetScreenToShadowMatrix(const SceneInfo& scene, uint32 TileOffsetX, uint32 TileOffsetY, uint32 TileResolutionX, uint32 TileResolutionY) const;

		wm::vector2<int> GetShadowBufferResolution() const;

		float GetShaderDepthBias() const
		{
			return ShaderDepthBias;
		}

		float GetShaderSlopeDepthBias() const
		{
			return ShaderSlopeDepthBias;
		}

		float GetShaderReceiverDepthBias() const
		{
			return 0.1f;
		}
	private:
		float ShaderDepthBias = 0;
		float ShaderSlopeDepthBias = 0;
		float ShaderMaxSlopeDepthBias = 0;
	};
}