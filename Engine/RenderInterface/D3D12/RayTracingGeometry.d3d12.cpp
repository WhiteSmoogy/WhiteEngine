#include "RayTracingGeometry.h"
#include "Context.h"
#include "Convert.h"
#include "NodeDevice.h"

namespace D12 = platform_ex::Windows::D3D12;
namespace R = platform::Render;

using namespace D12;
using std::shared_ptr;
using std::unique_ptr;

D12::RayTracingGeometry::RayTracingGeometry(const platform::Render::RayTracingGeometryInitializer& initializer)
{
	this->IndexBuffer = static_cast<GraphicsBuffer*>(initializer.IndexBuffer);

	white::uint32 IndexStride = this->IndexBuffer ? NumFormatBytes(Convert(this->IndexBuffer->GetFormat())) : 0;
	this->IndexStride = IndexStride;
	this->IndexOffsetInBytes = initializer.IndexBufferOffset;

	switch (initializer.GeometryType)
	{
	case R::ERayTracingGeometryType::Triangles:
		this->GeometryType = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		break;
	case R::ERayTracingGeometryType::Procedural:
		this->GeometryType = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		break;
	default:
		break;
	}

	bool fastBuild = false;
	bool allowUpdate = false;

	if (fastBuild)
	{
		this->BuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	}
	else
	{
		this->BuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	}

	if (allowUpdate)
	{
		this->BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	this->Segement = initializer.Segement;
}

void D12::RayTracingGeometry::BuildAccelerationStructure(CommandContext& CommandContext)
{
	wconstexpr white::uint32 IndicesPerPrimitive = 3; // Only triangle meshes are supported

	D3D12_RAYTRACING_GEOMETRY_DESC Desc;

	Desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	Desc.Type = GeometryType;

	bool allowAnyHitShader = false;
	bool allowDuplicateAnyHitShaderInvocation = true;

	if (!allowAnyHitShader)
	{
		Desc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	if (!allowDuplicateAnyHitShaderInvocation)
	{
		Desc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
	}

	if (GeometryType == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)
	{
		Desc.Triangles.VertexFormat = Convert(Segement.VertexFormat);

		Desc.Triangles.Transform3x4 = D3D12_GPU_VIRTUAL_ADDRESS(0);

		if (IndexBuffer)
		{
			Desc.Triangles.IndexFormat = IndexStride == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			Desc.Triangles.IndexCount = Segement.NumPrimitives * IndicesPerPrimitive;
			Desc.Triangles.IndexBuffer = IndexBuffer->GetGPUVirtualAddress() +
				IndexOffsetInBytes +
				IndexStride * Segement.FirstPrimitive * IndicesPerPrimitive;

			Desc.Triangles.VertexCount = Segement.VertexBuffer->GetSize() / Segement.VertexBufferStride;
		}
		else // Non-indexed geometry
		{
			Desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
			Desc.Triangles.IndexCount = 0;
			Desc.Triangles.IndexBuffer = D3D12_GPU_VIRTUAL_ADDRESS(0);

			Desc.Triangles.VertexCount = Segement.VertexBuffer->GetSize() / Segement.VertexBufferStride;
		}

		Desc.Triangles.VertexBuffer.StartAddress = Segement.VertexBuffer->GetGPUVirtualAddress() + Segement.VertexBufferOffset;
		Desc.Triangles.VertexBuffer.StrideInBytes = Segement.VertexBufferStride;
	}
	else if (GeometryType == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS)
	{
		//TODO
	}

	bool isUpdate = false;

	auto RayTracingDevice = CommandContext.GetParentDevice()->GetRayTracingDevice();
	
	auto LocalBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS(BuildFlags);

	if (isUpdate)
	{
		LocalBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS PrebuildDescInputs = {};

	PrebuildDescInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	PrebuildDescInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	PrebuildDescInputs.NumDescs =1;
	PrebuildDescInputs.pGeometryDescs = &Desc;
	PrebuildDescInputs.Flags = LocalBuildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
	RayTracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&PrebuildDescInputs, &PrebuildInfo);

	auto Adapter = CommandContext.GetParentAdapter();
	CreateAccelerationStructureBuffers(AccelerationStructureBuffer,ScratchBuffer, Adapter, PrebuildInfo, PrebuildDescInputs.Type);

	//scratch buffers should be created in UAV state from the start
	TransitionResource(CommandContext.CommandListHandle, ScratchBuffer->Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);
	//BuildRaytracingAccelerationStructure auto change resource state to UAV(is document?)
	CommandContext.CommandListHandle.FlushResourceBarriers();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC BuildDesc = {};
	BuildDesc.Inputs = PrebuildDescInputs;
	BuildDesc.DestAccelerationStructureData = AccelerationStructureBuffer->GetGPUVirtualAddress();
	BuildDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
	BuildDesc.SourceAccelerationStructureData = isUpdate
		? AccelerationStructureBuffer->GetGPUVirtualAddress()
		: D3D12_GPU_VIRTUAL_ADDRESS(0);

	auto RayTracingCommandList = CommandContext.CommandListHandle.RayTracingCommandList();
	RayTracingCommandList->BuildRaytracingAccelerationStructure(&BuildDesc, 0, nullptr);

	// We don't need to keep a scratch buffer after initial build if acceleration structure is static.
	if (!(BuildFlags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE))
	{
		ScratchBuffer.reset();
	}
}

using namespace R::Buffer;
namespace D3D = platform_ex::Windows::D3D;

void D12::CreateAccelerationStructureBuffers(shared_ptr<GraphicsBuffer>& AccelerationStructureBuffer, shared_ptr<GraphicsBuffer>& ScratchBuffer, D3D12Adapter* Creator, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& PrebuildInfo, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE Type)
{
	wconstraint(PrebuildInfo.ResultDataMaxSizeInBytes <= std::numeric_limits<uint32>::max());

	bool isTopLevel = Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	ResourceCreateInfo CreateInfo{
		.WithoutNativeResource = false,
		.DebugName = isTopLevel ? "Acceleration structure[Scene]" : "Acceleration structure[Geometry]",
		.ResouceData = nullptr
	};

	D3D12_RESOURCE_DESC AccelerationStructureBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		PrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	AccelerationStructureBuffer = platform::Render::shared_raw_robject(Creator->CreateBuffer<ResourceStateMode::Single>(
		nullptr, AccelerationStructureBufferDesc,Usage::Static,EAccessHint::EA_AccelerationStructure,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
		0, AccelerationStructureBufferDesc.Width, CreateInfo, nullptr));

	D3D12_RESOURCE_DESC ScratchBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		std::max(PrebuildInfo.UpdateScratchDataSizeInBytes, PrebuildInfo.ScratchDataSizeInBytes), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	CreateInfo.DebugName = isTopLevel ? "Acceleration structure scratch[Scene]" : "Acceleration structure scratch[Geometry]";
	ScratchBuffer = platform::Render::shared_raw_robject(Creator->CreateBuffer<ResourceStateMode::Single>(
		nullptr, ScratchBufferDesc, Usage::Static, EAccessHint::EA_GPUUnordered | EAccessHint::EA_Raw,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
		0, ScratchBufferDesc.Width, CreateInfo, nullptr));
}
