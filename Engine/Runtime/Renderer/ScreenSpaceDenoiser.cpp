#include "ScreenSpaceDenoiser.h"
#include "RenderInterface/BuiltInShader.h"
#include "Runtime/RenderCore/ShaderParamterTraits.hpp"
#include "Runtime/RenderCore/ShaderParameterStruct.h"
#include "Runtime/RenderCore/ShaderTextureTraits.hpp"
#include "RenderInterface/DrawEvent.h"
#include "RenderInterface/ShaderPermutation.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/RenderObject.h"

using namespace platform;

constexpr auto TILE_SIZE = 8;

/** Different signals to denoise. */
enum class SignalProcessing
{
	// Denoise a shadow mask.
	ShadowVisibilityMask,

	MAX
};

BEGIN_SHADER_PARAMETER_STRUCT(SSDCommonParameters)
	SHADER_PARAMETER(white::math::uint2, ViewportMin)
	SHADER_PARAMETER(white::math::uint2, ViewportMax)
	SHADER_PARAMETER(white::math::float4, ThreadIdToBufferUV)
	SHADER_PARAMETER(white::math::float4, BufferSizeAndInvSize)
	SHADER_PARAMETER(white::math::float4, BufferBilinearUVMinMax)
	SHADER_PARAMETER(white::math::float2, BufferUVToOutputPixelPosition)
	SHADER_PARAMETER(float, WorldDepthToPixelWorldRadius)
	SHADER_PARAMETER(float, HitDistanceToWorldBluringRadius)
	SHADER_PARAMETER(white::math::float4, BufferUVToScreenPosition)
END_SHADER_PARAMETER_STRUCT();

/** Returns whether a signal processing support upscaling. */
static bool SignalSupportsUpscaling(SignalProcessing SignalProcessing)
{
	return false;
}

static bool SignalUsesPreConvolution(SignalProcessing SignalProcessing)
{
	return
		SignalProcessing == SignalProcessing::ShadowVisibilityMask;
}

/** Returns whether a signal processing uses a history rejection pre convolution pass. */
static bool SignalUsesRejectionPreConvolution(SignalProcessing SignalProcessing)
{
	return false;
}

/** Returns whether a signal processing uses a convolution pass after temporal accumulation pass. */
static bool SignalUsesPostConvolution(SignalProcessing SignalProcessing)
{
	return false;
}

/** Returns whether a signal processing uses a history rejection pre convolution pass. */
static bool SignalUsesFinalConvolution(SignalProcessing SignalProcessing)
{
	return SignalProcessing == SignalProcessing::ShadowVisibilityMask;
}


/** Returns whether a signal can denoise multi sample per pixel. */
static bool SignalSupportMultiSPP(SignalProcessing SignalProcessing)
{
	return (
		SignalProcessing == SignalProcessing::ShadowVisibilityMask
		);
}

/** Returns whether a signal have a code path for 1 sample per pixel. */
static bool SignalSupport1SPP(SignalProcessing SignalProcessing)
{
	return (
		false);
}


// Permutation dimension for the type of signal being denoised.
class FSignalProcessingDim : SHADER_PERMUTATION_ENUM_CLASS("DIM_SIGNAL_PROCESSING", SignalProcessing);

// Permutation dimension for the number of signal being denoised at the same time.
class FSignalBatchSizeDim : SHADER_PERMUTATION_RANGE_INT("DIM_SIGNAL_BATCH_SIZE", 1, 4);

// Permutation dimension for denoising multiple sample at same time.
class FMultiSPPDim : SHADER_PERMUTATION_BOOL("DIM_MULTI_SPP");

class SSDSpatialAccumulationCS : public Render::BuiltInShader
{
public:
	enum class Stage
	{
		// Spatial kernel used to process raw input for the temporal accumulation.
		ReConstruction,

		// Spatial kernel to pre filter.
		PreConvolution,

		// Spatial kernel used to pre convolve history rejection.
		RejectionPreConvolution,

		// Spatial kernel used to post filter the temporal accumulation.
		PostFiltering,

		// Final spatial kernel, that may output specific buffer encoding to integrate with the rest of the renderer
		FinalOutput,

		MAX
	};

	class FStageDim : SHADER_PERMUTATION_ENUM_CLASS("DIM_STAGE", Stage);
	class FUpscaleDim : SHADER_PERMUTATION_BOOL("DIM_UPSCALE");

	using FPermutationDomain = Render::TShaderPermutationDomain<FSignalProcessingDim, FStageDim, FUpscaleDim, FSignalBatchSizeDim, FMultiSPPDim>;

	static bool ShouldCompilePermutation(const Render::FBuiltInShaderPermutationParameters& Parameters)
	{
		FPermutationDomain PermutationVector(Parameters.PermutationId);
		SignalProcessing SignalProcessing = PermutationVector.Get<FSignalProcessingDim>();

		// Only reconstruction have upscale capability for now.
		if (PermutationVector.Get<FUpscaleDim>() &&
			PermutationVector.Get<FStageDim>() != Stage::ReConstruction)
		{
			return false;
		}

		// Only upscale is only for signal that needs it.
		if (PermutationVector.Get<FUpscaleDim>() &&
			!SignalSupportsUpscaling(SignalProcessing))
		{
			return false;
		}

		// Only compile pre convolution for signal that uses it.
		if (!SignalUsesPreConvolution(SignalProcessing) &&
			PermutationVector.Get<FStageDim>() == Stage::PreConvolution)
		{
			return false;
		}

		// Only compile rejection pre convolution for signal that uses it.
		if (!SignalUsesRejectionPreConvolution(SignalProcessing) &&
			PermutationVector.Get<FStageDim>() == Stage::RejectionPreConvolution)
		{
			return false;
		}

		// Only compile post convolution for signal that uses it.
		if (!SignalUsesPostConvolution(SignalProcessing) &&
			PermutationVector.Get<FStageDim>() == Stage::PostFiltering)
		{
			return false;
		}

		// Only compile final convolution for signal that uses it.
		if (!SignalUsesFinalConvolution(SignalProcessing) &&
			PermutationVector.Get<FStageDim>() == Stage::FinalOutput)
		{
			return false;
		}

		// Only compile multi SPP permutation for signal that supports it.
		if (PermutationVector.Get<FStageDim>() == Stage::ReConstruction &&
			PermutationVector.Get<FMultiSPPDim>() && !SignalSupportMultiSPP(SignalProcessing))
		{
			return false;
		}

		// Compile out the shader if this permutation gets remapped.
		if (RemapPermutationVector(PermutationVector) != PermutationVector)
		{
			return false;
		}

		return true;
	}

	static FPermutationDomain RemapPermutationVector(FPermutationDomain PermutationVector)
	{
		SignalProcessing SignalProcessing = PermutationVector.Get<FSignalProcessingDim>();

		if (PermutationVector.Get<FStageDim>() == Stage::ReConstruction)
		{
			// force use the multi sample per pixel code path.
			if (!SignalSupport1SPP(SignalProcessing))
			{
				PermutationVector.Set<FMultiSPPDim>(true);
			}
		}
		else
		{
			PermutationVector.Set<FMultiSPPDim>(true);
		}

		return PermutationVector;
	}


	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(SSDCommonParameters, CommonParameters)
		//TODO:ViewUniformBuffer
		SHADER_PARAMETER(white::uint32, StateFrameIndexMod8)
		SHADER_PARAMETER(white::math::float4x4, ScreenToTranslatedWorld)
		SHADER_PARAMETER(white::math::float4x4, ViewToClip)
		SHADER_PARAMETER(white::math::float4x4, TranslatedWorldToView)
		SHADER_PARAMETER(white::math::float4, InvDeviceZToWorldZTransform)
		//TODO:SceneParamets
		SHADER_PARAMETER_TEXTURE(Render::Texture2D, SceneDepthBuffer)
		SHADER_PARAMETER_SAMPLER(Render::TextureSampleDesc, SceneDepthBufferSampler)
		SHADER_PARAMETER_TEXTURE(Render::Texture2D, WorldNormalBuffer)
		SHADER_PARAMETER_SAMPLER(Render::TextureSampleDesc, WorldNormalSampler)

		SHADER_PARAMETER_SAMPLER(Render::TextureSampleDesc, GlobalPointClampedSampler)

		//SignalFramework.h
		SHADER_PARAMETER(float, HarmonicPeriode)
		SHADER_PARAMETER(white::uint32, UpscaleFactor)
		SHADER_PARAMETER(white::math::float4, InputBufferUVMinMax)
		SHADER_PARAMETER(white::uint32, MaxSampleCount)

		SHADER_PARAMETER(float, KernelSpreadFactor)

		SHADER_PARAMETER_TEXTURE(Render::Texture2D, SignalInput_Textures_0)
		SHADER_PARAMETER_TEXTURE(Render::Shader::RWTexture2D, SignalOutput_UAVs_0)
		
		END_SHADER_PARAMETER_STRUCT();

	EXPORTED_BUILTIN_SHADER(SSDSpatialAccumulationCS);
};

IMPLEMENT_BUILTIN_SHADER(SSDSpatialAccumulationCS, "SSD/Shadow/SSDSpatialAccumulation.wsl", "MainCS", platform::Render::ComputeShader);


namespace Shadow
{
	using namespace platform::Render;

	class SSDInjestCS : public BuiltInShader
	{
	public:
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(SSDCommonParameters, CommonParameters)
			SHADER_PARAMETER(white::math::float4, InputBufferUVMinMax)
			SHADER_PARAMETER_TEXTURE(Render::Texture2D, SignalInput_Textures_0)
			SHADER_PARAMETER_TEXTURE(Render::Shader::RWTexture2D, SignalOutput_UAVs_0)
			SHADER_PARAMETER_SAMPLER(Render::TextureSampleDesc, point_sampler)
			END_SHADER_PARAMETER_STRUCT();

		EXPORTED_BUILTIN_SHADER(SSDInjestCS);
	};

	IMPLEMENT_BUILTIN_SHADER(SSDInjestCS, "SSD/Shadow/SSDInjest.wsl", "MainCS", platform::Render::ComputeShader);

}

constexpr auto kStackowiakMaxSampleCountPerSet = 56;


void platform::ScreenSpaceDenoiser::DenoiseShadowVisibilityMasks(Render::CommandList& CmdList, const ShadowViewInfo& ViewInfo, const ShadowVisibilityInput& InputParameters, const ShadowVisibilityOutput& Output)
{
	constexpr auto ReconstructionSamples = 8;

	auto& Device = Render::Context::Instance().GetDevice();
	auto FullResW = InputParameters.Mask->GetWidth(0);
	auto FullResH = InputParameters.Mask->GetHeight(0);

	SSDCommonParameters CommonParameters;
	{
		CommonParameters.ViewportMin = white::math::uint2(0, 0);
		CommonParameters.ViewportMax = white::math::uint2(FullResW, FullResH);

		CommonParameters.ThreadIdToBufferUV.x = 1.0f / FullResW;
		CommonParameters.ThreadIdToBufferUV.y = 1.0f / FullResH;
		CommonParameters.ThreadIdToBufferUV.z = 0.5f / FullResW;
		CommonParameters.ThreadIdToBufferUV.w = 0.5f / FullResH;

		CommonParameters.BufferBilinearUVMinMax = white::math::float4(
			0.5f / FullResW, 0.5f / FullResH,
			(FullResW - 0.5f) / FullResW, (FullResH - 0.5f) / FullResH
		);
		CommonParameters.BufferSizeAndInvSize = white::math::float4(FullResW, FullResH, 1.0f / FullResW, 1.0f / FullResH);

		CommonParameters.BufferUVToOutputPixelPosition = white::math::float2(FullResW, FullResH);

		CommonParameters.BufferUVToScreenPosition.x = 2;
		CommonParameters.BufferUVToScreenPosition.y = -2;
		CommonParameters.BufferUVToScreenPosition.z = -1.0f;
		CommonParameters.BufferUVToScreenPosition.w = 1.0f;
		CommonParameters.HitDistanceToWorldBluringRadius = std::tanf(InputParameters.LightHalfRadians);

		float TanHalfFieldOfView = ViewInfo.InvProjectionMatrix[0][0];

		CommonParameters.WorldDepthToPixelWorldRadius = TanHalfFieldOfView / FullResW;
	}

	auto SignalHistroy = InputParameters.Mask;
	//Injest
	{
		SCOPED_GPU_EVENT(CmdList, ShadowInjest);
		auto injeset = Render::shared_raw_robject(Device.CreateTexture(FullResW, FullResH,

			1, 1, Render::EF_R32UI, Render::EA_GPURead | Render::EA_GPUUnordered | Render::EA_GPUWrite, {}));
		auto InjestShader = Render::GetBuiltInShaderMap()->GetShader<Shadow::SSDInjestCS>();

		Shadow::SSDInjestCS::Parameters Parameters;
		
		Parameters.CommonParameters = CommonParameters;
		Parameters.InputBufferUVMinMax = CommonParameters.BufferBilinearUVMinMax;

		Parameters.SignalInput_Textures_0 = SignalHistroy;
		auto uav = Render::shared_raw_robject(Device.CreateUnorderedAccessView(injeset.get()));
		Parameters.SignalOutput_UAVs_0 = uav.get();

		Parameters.point_sampler = Render::TextureSampleDesc::point_sampler;

		ComputeShaderUtils::Dispatch(CmdList, InjestShader, Parameters,
			ComputeShaderUtils::GetGroupCount(white::math::int2(FullResW,FullResH),TILE_SIZE));

		SignalHistroy = injeset.get();
	}

	auto setup_common_parameters = [&](SSDSpatialAccumulationCS::Parameters& Parameters)
	{
		Parameters.CommonParameters = CommonParameters;


		Parameters.InputBufferUVMinMax = CommonParameters.BufferBilinearUVMinMax;

		Parameters.StateFrameIndexMod8 = ViewInfo.StateFrameIndex % 8;
		Parameters.ScreenToTranslatedWorld = white::math::transpose(ViewInfo.ScreenToTranslatedWorld);
		Parameters.ViewToClip = white::math::transpose(ViewInfo.ViewToClip);
		Parameters.TranslatedWorldToView = white::math::transpose(ViewInfo.TranslatedWorldToView);
		Parameters.InvDeviceZToWorldZTransform = ViewInfo.InvDeviceZToWorldZTransform;

		//SceneParams
		Parameters.SceneDepthBuffer = static_cast<Render::Texture2D*>(InputParameters.SceneDepth);
		Parameters.WorldNormalBuffer = InputParameters.WorldNormal;

		Parameters.GlobalPointClampedSampler = Render::TextureSampleDesc::point_sampler;
		Parameters.SceneDepthBufferSampler = Render::TextureSampleDesc::point_sampler;
		Parameters.WorldNormalSampler = Render::TextureSampleDesc::point_sampler;
	};

	// Spatial reconstruction with ratio estimator to be more precise in the history rejection.
	{
		SCOPED_GPU_EVENT(CmdList, SSDSpatialAccumulation_Reconstruction);

		auto spatial_reconst = Render::shared_raw_robject(Device.CreateTexture(FullResW, FullResH,
			1, 1, Render::EF_ABGR16F, Render::EA_GPURead | Render::EA_GPUUnordered | Render::EA_GPUWrite, {}));

		SSDSpatialAccumulationCS::Parameters Parameters;

		setup_common_parameters(Parameters);

		Parameters.MaxSampleCount = std::max(std::min(ReconstructionSamples, kStackowiakMaxSampleCountPerSet), 1);
		Parameters.UpscaleFactor = 1;
		Parameters.HarmonicPeriode = 1;


		auto uav = Render::shared_raw_robject(Device.CreateUnorderedAccessView(spatial_reconst.get()));
		Parameters.SignalInput_Textures_0 = SignalHistroy;
		Parameters.SignalOutput_UAVs_0 = uav.get();


		SSDSpatialAccumulationCS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FSignalProcessingDim>(SignalProcessing::ShadowVisibilityMask);
		PermutationVector.Set<FSignalBatchSizeDim>(1);
		PermutationVector.Set<SSDSpatialAccumulationCS::FStageDim>(SSDSpatialAccumulationCS::Stage::ReConstruction);
		PermutationVector.Set<SSDSpatialAccumulationCS::FUpscaleDim>(false);
		PermutationVector.Set<FMultiSPPDim>(true);

		Render::ShaderMapRef<SSDSpatialAccumulationCS> ComputeShader(Render::GetBuiltInShaderMap(), SSDSpatialAccumulationCS::RemapPermutationVector(PermutationVector));

		ComputeShaderUtils::Dispatch(CmdList, ComputeShader, Parameters,
			ComputeShaderUtils::GetGroupCount(white::math::int2(FullResW, FullResH), TILE_SIZE));

		SignalHistroy = spatial_reconst.get();
	}

	const int PreConvolutionCount = 1;
	// Spatial pre convolutions
	for (int PreConvolutionId = 0; PreConvolutionId < PreConvolutionCount; PreConvolutionId++)
	{

		auto convolution = Render::shared_raw_robject(Device.CreateTexture(FullResW, FullResH,
			1, 1, Render::EF_ABGR16F, Render::EA_GPURead | Render::EA_GPUUnordered | Render::EA_GPUWrite, {}));

		SSDSpatialAccumulationCS::Parameters Parameters;

		setup_common_parameters(Parameters);
		Parameters.KernelSpreadFactor =static_cast<float>(8 * (1 << PreConvolutionId));

		auto uav = Render::shared_raw_robject(Device.CreateUnorderedAccessView(convolution.get()));
		Parameters.SignalInput_Textures_0 = SignalHistroy;
		Parameters.SignalOutput_UAVs_0 = uav.get();

		SSDSpatialAccumulationCS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FSignalProcessingDim>(SignalProcessing::ShadowVisibilityMask);
		PermutationVector.Set<FSignalBatchSizeDim>(1);
		PermutationVector.Set<SSDSpatialAccumulationCS::FStageDim>(SSDSpatialAccumulationCS::Stage::PreConvolution);
		PermutationVector.Set<FMultiSPPDim>(true);

		Render::ShaderMapRef<SSDSpatialAccumulationCS> ComputeShader(Render::GetBuiltInShaderMap(), PermutationVector);

		SCOPED_GPU_EVENTF(CmdList, "PreConvolution(ConvolutionId=%d Spread=%f)", PreConvolutionId, Parameters.KernelSpreadFactor);

		ComputeShaderUtils::Dispatch(CmdList, ComputeShader, Parameters,
			ComputeShaderUtils::GetGroupCount(white::math::int2(FullResW, FullResH), TILE_SIZE));

		SignalHistroy = convolution.get();
	}

	//TODO:Temporal

	//Final convolution
	{
		SSDSpatialAccumulationCS::Parameters Parameters;

		setup_common_parameters(Parameters);
		Parameters.SignalInput_Textures_0 = SignalHistroy;
		Parameters.SignalOutput_UAVs_0 = Output.MaskUAV;

		SSDSpatialAccumulationCS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FSignalProcessingDim>(SignalProcessing::ShadowVisibilityMask);
		PermutationVector.Set<FSignalBatchSizeDim>(1);
		PermutationVector.Set<SSDSpatialAccumulationCS::FStageDim>(SSDSpatialAccumulationCS::Stage::FinalOutput);
		PermutationVector.Set<FMultiSPPDim>(true);

		Render::ShaderMapRef<SSDSpatialAccumulationCS> ComputeShader(Render::GetBuiltInShaderMap(), PermutationVector);

		SCOPED_GPU_EVENTF(CmdList, "SpatialAccumulation(Final)");

		ComputeShaderUtils::Dispatch(CmdList, ComputeShader, Parameters,
			ComputeShaderUtils::GetGroupCount(white::math::int2(FullResW, FullResH), TILE_SIZE));
	}
}
