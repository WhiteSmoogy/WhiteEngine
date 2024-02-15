#pragma once

#include "ShaderParameters.h"

namespace platform::Render {
	template<typename TCommandList,typename TShaderClass, THardwareShader TShader>
	inline void SetShaderParameters(TCommandList& cmdlist, const ShaderRef<TShaderClass>& Shader, TShader* ShaderRHI, const typename TShaderClass::Parameters& Parameters)
	{
		//ValidateShaderParameters

		wconstraint(ShaderRHI != nullptr);
		const auto& Bindings = Shader->Bindings;

		auto ParametersPtr = &Parameters;
		const uint8* Base = reinterpret_cast<const uint8*>(ParametersPtr);

		// Parameters
		for (auto& ParameterBinding : Bindings.Parameters)
		{
			const void* DataPtr = Base + ParameterBinding.ByteOffset;
			cmdlist.SetShaderParameter(ShaderRHI, ParameterBinding.BufferIndex, ParameterBinding.BaseIndex, ParameterBinding.ByteSize,DataPtr);
		}

		// Textures
		for (auto& TextureBinding : Bindings.Textures)
		{
			auto ShaderParameterRef = *(Texture**)(Base + TextureBinding.ByteOffset);

			cmdlist.SetShaderTexture(ShaderRHI, TextureBinding.BaseIndex, ShaderParameterRef);
		}

		// SRV
		for (auto& SRVBinding : Bindings.SRVs)
		{
			auto ShaderParameterRef = *(ShaderResourceView**)(Base + SRVBinding.ByteOffset);

			cmdlist.SetShaderResourceView(ShaderRHI, SRVBinding.BaseIndex, ShaderParameterRef);
		}

		// ConstantBuffer
		for (auto& CBBinding : Bindings.Constants)
		{
			auto ShaderParameterRef = *(ConstantBuffer**)(Base + CBBinding.ByteOffset);

			cmdlist.SetShaderConstantBuffer(ShaderRHI, CBBinding.BaseIndex, ShaderParameterRef);
		}

		//Samplers
		for (auto& SamplerBinding : Bindings.Samplers)
		{
			auto ShaderParameterRef = *(TextureSampleDesc*)(Base + SamplerBinding.ByteOffset);

			cmdlist.SetShaderSampler(ShaderRHI, SamplerBinding.BaseIndex, ShaderParameterRef);
		}

		if constexpr (std::is_same_v<TShader, ComputeHWShader>)
		{
			//UAVS
			for (auto& UAVBinding : Bindings.UAVs)
			{
				auto ShaderParameterRef = *(UnorderedAccessView**)(Base + UAVBinding.ByteOffset);

				cmdlist.SetUAVParameter(ShaderRHI, UAVBinding.BaseIndex, ShaderParameterRef);
			}
		}
	}

	/** Set shader's parameters from its parameters struct. */
	template<typename TShaderClass>
	void SetShaderParameters(RayTracingShaderBindingsWriter& RTBindingsWriter, const ShaderRef<TShaderClass>& Shader, const typename TShaderClass::Parameters& Parameters)
	{
		const auto& Bindings = Shader->Bindings;

		WAssert(Bindings.Parameters.empty(), "Ray tracing shader should use SHADER_USE_ROOT_PARAMETER_STRUCT() to passdown the cbuffer layout to the shader compiler.");

		auto ParametersPtr = &Parameters;
		const uint8* Base = reinterpret_cast<const uint8*>(ParametersPtr);


		// Textures
		for (auto& TextureBinding : Bindings.Textures)
		{
			auto ShaderParameterRef = *(Texture**)(Base + TextureBinding.ByteOffset);

			RTBindingsWriter.SetTexture(TextureBinding.BaseIndex, ShaderParameterRef);
		}

		//Samplers
		for (auto& SamplerBinding : Bindings.Samplers)
		{
			auto ShaderParameterRef = (TextureSampleDesc*)(Base + SamplerBinding.ByteOffset);

			RTBindingsWriter.SetShaderSampler(SamplerBinding.BaseIndex, ShaderParameterRef);
		}

		
		//UAVS
		for (auto& UAVBinding : Bindings.UAVs)
		{
			auto ShaderParameterRef = *(UnorderedAccessView**)(Base + UAVBinding.ByteOffset);

			RTBindingsWriter.SetUAVParameter(UAVBinding.BaseIndex, ShaderParameterRef);
		}
		
		// Root uniform buffer.
		if (Bindings.RootParameterBufferIndex != RenderShaderParameterBindings::kInvalidBufferIndex)
		{
			RTBindingsWriter.RootUniformBuffer = CreateGraphicsBuffeImmediate(Parameters, Buffer::Usage::SingleDraw);
			RTBindingsWriter.SetGraphicsBuffer(Bindings.RootParameterBufferIndex, RTBindingsWriter.RootUniformBuffer.get());
		}
	}
}
