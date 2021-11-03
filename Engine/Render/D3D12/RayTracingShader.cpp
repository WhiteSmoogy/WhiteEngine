#include "RayTracingShader.h"
#include "Context.h"
#include "Core/Serialization/MemoryReader.h"
#include <WFramework/Core/WString.h>

using namespace platform_ex::Windows::D3D12;
using namespace platform::Render::Shader;

void QuantizeBoundShaderState(
	ShaderType Type,
	const D3D12_RESOURCE_BINDING_TIER& ResourceBindingTier,
	const RayTracingShader* const RayTracingShader,
	QuantizedBoundShaderState& OutQBSS
)
{
	wconstraint(RayTracingShader);

	const auto& Counts = RayTracingShader->ResourceCounts;

	std::memset(&OutQBSS,0, sizeof(OutQBSS));
	auto& QBSSRegisterCounts = OutQBSS.RegisterCounts[platform::Render::VisibilityAll];

	switch (Type)
	{
	case RayGen:
	case RayMiss:

		// Shared conservative root signature layout is used for all raygen and miss shaders.

		OutQBSS.RootSignatureType = RootSignatureType::RayTracingGlobal;

		QBSSRegisterCounts.NumSamplers = MAX_SAMPLERS;
		QBSSRegisterCounts.NumSRVs = MAX_SRVS;
		QBSSRegisterCounts.NumCBs = MAX_CBS;
		QBSSRegisterCounts.NumUAVs = MAX_UAVS;

		break;

	case RayHitGroup:
	case RayCallable:

		// Local root signature is used for hit group shaders, using the exact number of resources to minimize shader binding table record size.

		OutQBSS.RootSignatureType = RootSignatureType::RayTracingLocal;

		QBSSRegisterCounts.NumSamplers = Counts.NumSamplers;
		QBSSRegisterCounts.NumSRVs = Counts.NumSRVs;
		QBSSRegisterCounts.NumCBs = Counts.NumCBs;
		QBSSRegisterCounts.NumUAVs = Counts.NumUAVs;

		wconstraint(QBSSRegisterCounts.NumSamplers <= MAX_SAMPLERS);
		wconstraint(QBSSRegisterCounts.NumSRVs <= MAX_SRVS);
		wconstraint(QBSSRegisterCounts.NumCBs <= MAX_CBS);
		wconstraint(QBSSRegisterCounts.NumUAVs <= MAX_UAVS);

		break;
	default:
		wassume(false); // Unexpected shader target frequency
	}
}

RayTracingShader::RayTracingShader(const white::span<const uint8>& Code)
	:D3D12HardwareShader()
{
	ShaderCodeReader ShaderCode(Code);

	WhiteEngine::MemoryReaderView Ar(Code);
	std::string RayEntryPoint;
	std::string RayAnyHitEntryPoint;
	std::string RayIntersectionEntryPoint;
	Ar >> RayEntryPoint;
	Ar >> RayAnyHitEntryPoint;
	Ar >> RayIntersectionEntryPoint;

	int32 Offset = 0;

	auto CodePtr = Code.data();
	auto CodeSize = ShaderCode.GetActualShaderCodeSize() - Offset;

	ShaderByteCode.first = std::make_unique<byte[]>(CodeSize);
	ShaderByteCode.second = CodeSize;
	std::memcpy(ShaderByteCode.first.get(), CodePtr, CodeSize);

	auto PackedResourceCounts = ShaderCode.FindOptionalData<ShaderCodeResourceCounts>();
	wassume(PackedResourceCounts);
	ResourceCounts = *PackedResourceCounts;

	auto Type = reinterpret_cast<const platform::Render::ShaderType*>(ShaderCode.FindOptionalData('t'));
	wconstraint(Type);

	auto& Device = Context::Instance().GetDevice();

	const D3D12_RESOURCE_BINDING_TIER Tier = Device.GetResourceBindingTier();
	QuantizedBoundShaderState QBSS;
	QuantizeBoundShaderState(*Type, Tier, this, QBSS);

	pRootSignature = Device.CreateRootSignature(QBSS);

	EntryPoint =white::Text::String(RayEntryPoint);
	AnyHitEntryPoint = white::Text::String(RayAnyHitEntryPoint);
	IntersectionEntryPoint = white::Text::String(RayIntersectionEntryPoint);
}