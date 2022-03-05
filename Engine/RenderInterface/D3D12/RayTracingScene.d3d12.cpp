#include "RayTracingScene.h"
#include "RayTracingGeometry.h"
#include "CommandContext.h"
#include "NodeDevice.h"
#include "Adapter.h"
#include "Context.h"

using namespace platform_ex::Windows::D3D12;
using namespace platform::Render::Buffer;

using std::shared_ptr;
using std::unique_ptr;

RayTracingScene::RayTracingScene(const platform::Render::RayTracingSceneInitializer& initializer)
	:Instances(initializer.Instances.begin(),initializer.Instances.end()),
	ShaderSlotsPerGeometrySegment(initializer.ShaderSlotsPerGeometrySegment),
	NumCallableShaderSlots(initializer.NumCallableShaderSlots)
{
}

void RayTracingScene::BuildAccelerationStructure(CommandContext& CommandContext)
{
	auto BuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	uint32 NumDxrInstances =static_cast<uint32>(Instances.size());

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS PrebuildDescInputs = {};

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
	PrebuildDescInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	PrebuildDescInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	PrebuildDescInputs.NumDescs = NumDxrInstances;
	PrebuildDescInputs.Flags = BuildFlags;

	auto RayTracingDevice = CommandContext.GetParentDevice()->GetRayTracingDevice();
	RayTracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&PrebuildDescInputs, &PrebuildInfo);

	auto Adapter = CommandContext.GetParentAdapter();

	shared_ptr<GraphicsBuffer> ScratchBuffer;
	CreateAccelerationStructureBuffers(AccelerationStructureBuffer, ScratchBuffer, Adapter, PrebuildInfo, PrebuildDescInputs.Type);

	//scratch buffers should be created in UAV state from the start
	TransitionResource(CommandContext.CommandListHandle, ScratchBuffer->Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);

	AccelerationStructureView = white::make_observer(AccelerationStructureBuffer->RetriveShaderResourceView());

	// Create and fill instance buffer
	const uint32 NumSceneInstances =static_cast<uint32>(Instances.size());

	unique_ptr<GraphicsBuffer> InstanceBuffer;

	if (NumSceneInstances)
	{
		InstanceBuffer =white::unique_raw(Adapter->CreateVertexBuffer(Usage::Static, EAccessHint::EA_CPUWrite,
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * PrebuildDescInputs.NumDescs,
			EFormat::EF_Unknown));
		InstanceBuffer->SetName("Acceleration structure [Instance] Init");
		{
			Mapper mapper(*InstanceBuffer, Access::Write_Only);

			auto MappedData = mapper.Pointer< D3D12_RAYTRACING_INSTANCE_DESC>();

			WAssertNonnull(MappedData);

			for (uint32 InstanceIndex = 0; InstanceIndex < NumSceneInstances; ++InstanceIndex)
			{
				auto& Instance = Instances[InstanceIndex];

				auto Geometry = static_cast<RayTracingGeometry*>(Instance.Geometry);

				D3D12_RAYTRACING_INSTANCE_DESC InstanceDesc = {};

				InstanceDesc.InstanceMask = Instance.Mask;
				InstanceDesc.InstanceContributionToHitGroupIndex = InstanceIndex * ShaderSlotsPerGeometrySegment;

				InstanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

				bool bForceOpaque = false;
				if (bForceOpaque)
				{
					InstanceDesc.Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
				}

				bool bDoubleSided = false;
				if (bDoubleSided)
				{
					InstanceDesc.Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
				}

				InstanceDesc.AccelerationStructure = Geometry->AccelerationStructureBuffer->Resource()->GetGPUVirtualAddress();

				InstanceDesc.InstanceID = 0;

				auto TransformTransposed = white::math::transpose(Instance.Transform);

				memcpy(InstanceDesc.Transform, TransformTransposed);

				MappedData[InstanceIndex] = InstanceDesc;
			}
		}

		InstanceBuffer->SetName("Acceleration structure [Instance] Fill");
		Context::Instance().ResidencyResource(InstanceBuffer->D3DResource());
	}

	const bool IsUpdateMode = false;

	Context::Instance().ResidencyResource(ScratchBuffer->D3DResource());
	//AccelerationStructureBuffer build per frame
	Context::Instance().ResidencyResource(AccelerationStructureBuffer->D3DResource());

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC BuildDesc = {};
	BuildDesc.Inputs = PrebuildDescInputs;
	BuildDesc.Inputs.InstanceDescs = InstanceBuffer->Resource()->GetGPUVirtualAddress();
	BuildDesc.DestAccelerationStructureData = AccelerationStructureBuffer->Resource()->GetGPUVirtualAddress();
	BuildDesc.ScratchAccelerationStructureData = ScratchBuffer->Resource()->GetGPUVirtualAddress();
	BuildDesc.SourceAccelerationStructureData = D3D12_GPU_VIRTUAL_ADDRESS(0);

	CommandContext.CommandListHandle.AddUAVBarrier();
	CommandContext.CommandListHandle.FlushResourceBarriers();

	auto RayTracingCommandList = CommandContext.CommandListHandle.RayTracingCommandList();

	RayTracingCommandList->BuildRaytracingAccelerationStructure(&BuildDesc, 0, nullptr);

	CommandContext.CommandListHandle.AddUAVBarrier();
}
