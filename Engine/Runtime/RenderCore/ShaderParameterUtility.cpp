#pragma once
#include "RenderInterface/Shader.h"
#include "ShaderParametersMetadata.h"
#include <WBase/cformat.h>

using namespace platform::Render::Shader;
using namespace platform::Render;

struct ShaderParameterStructBinding
{
	const RenderShader* Shader;

	RenderShaderParameterBindings* Bindings;

	const ShaderParameterMap* ParametersMap;

	bool RootShaderParameters = false;

	void Bind(const ShaderParametersMetadata& StructMetaData,uint32 GeneralByteOffset)
	{
		auto& StructMembers = StructMetaData.GetMembers();

		for (auto& Member : StructMembers)
		{
			auto ShaderBindingName = white::sfmt("%s", Member.GetName());

			auto ByteOffset = GeneralByteOffset + static_cast<uint16>(Member.GetOffset());

			auto ShaderType = Member.GetShaderType();

			const bool bIsVariableNativeType = IsNumberType(ShaderType);

			if (ShaderType == SPT_StructInclude)
			{
				Bind(*Member.GetStructMetadata(), ByteOffset);
				continue;
			}

			if (RootShaderParameters && bIsVariableNativeType)
				continue;

			uint16 BufferIndex, BaseIndex, BoundSize;
			if (!ParametersMap->FindParameterAllocation(ShaderBindingName, BufferIndex, BaseIndex, BoundSize))
			{
				continue;
			}

			if (bIsVariableNativeType)
			{
				RenderShaderParameterBindings::Parameter Parameter;
				Parameter.BufferIndex = BufferIndex;
				Parameter.BaseIndex = BaseIndex;
				Parameter.ByteOffset = ByteOffset;
				Parameter.ByteSize = BoundSize;

				Bindings->Parameters.emplace_back(Parameter);
			}

			RenderShaderParameterBindings::ResourceParameter Parameter;
			Parameter.BaseIndex = BaseIndex;
			Parameter.ByteOffset = ByteOffset;
			const bool bIsSRVType = GetBaseType(ShaderType) == SBT_SRV;

			if (bIsSRVType)
			{
				const bool bIsTextureType = IsTextureReadType(ShaderType);

				if (bIsTextureType)
				{
					Bindings->Textures.emplace_back(Parameter);
				}
				else
				{
					Bindings->SRVs.emplace_back(Parameter);
				}
			}

			if (ShaderType == SPT_sampler)
			{
				Bindings->Samplers.emplace_back(Parameter);
			}

			if (ShaderType == SPT_ConstantBuffer)
			{
				Bindings->Constants.emplace_back(Parameter);
			}

			const bool bIsUAVType = GetBaseType(ShaderType) == SBT_UAV;

			if (bIsUAVType)
			{
				Bindings->UAVs.emplace_back(Parameter);
			}
		}
	}
};

void RenderShaderParameterBindings::BindForLegacyShaderParameters(const RenderShader* Shader, const ShaderParameterMap& ParameterMaps, const ShaderParametersMetadata& StructMetaData)
{
	auto Meta = Shader->GetMeta();

	switch (Meta->GetShaderType())
	{
	case VertexShader:
	case PixelShader:
	case HullShader:
	case DomainShader:
	case GeometryShader:
	case ComputeShader:
		break;
	default:
		WAssert(false, "Invalid shader frequency for this shader binding technique.");
	}

	ShaderParameterStructBinding Binding;
	Binding.Shader = Shader;
	Binding.Bindings = this;
	Binding.ParametersMap = &ParameterMaps;

	Binding.Bind(StructMetaData,0);

	RootParameterBufferIndex = kInvalidBufferIndex;

}

void RenderShaderParameterBindings::BindForRootShaderParameters(const RenderShader* Shader, const ShaderParameterMap& ParameterMaps)
{
	//WAssert(StructMetaData.GetSize() < (1<<16),"Shader parameter structure can only have a size < 65536 bytes.");

	auto Meta = Shader->GetMeta();
	auto& StructMetaData = *Meta->GetRootParametersMetadata();

	switch (Meta->GetShaderType())
	{
	case RayGen:
	case RayMiss:
	case RayHitGroup:
	case RayCallable:
		break;
	default:
		WAssert(false, "Invalid shader frequency for this shader binding technique.");
	}

	ShaderParameterStructBinding Binding;
	Binding.Shader = Shader;
	Binding.Bindings = this;
	Binding.ParametersMap = &ParameterMaps;
	Binding.RootShaderParameters = true;

	Binding.Bind(StructMetaData,0);

	uint16 BufferIndex, BaseIndex, BoundSize;
	if (ParameterMaps.FindParameterAllocation(ShaderParametersMetadata::kRootUniformBufferBindingName, BufferIndex, BaseIndex, BoundSize))
	{
		RootParameterBufferIndex = BufferIndex;
	}
	else
	{
		wassume(RootParameterBufferIndex == kInvalidBufferIndex);
	}
}