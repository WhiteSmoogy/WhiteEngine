/*! \file Engine\Render\D3D12\RayTracingGeometry.h
\ingroup Engine
\brief ���߼�����Ϣʵ���ࡣ
*/
#ifndef WE_RENDER_D3D12_RayTracingGeometry_h
#define WE_RENDER_D3D12_RayTracingGeometry_h 1

#include "RenderInterface/IRayTracingGeometry.h"
#include "RenderInterface/IRayDevice.h"
#include "d3d12_dxgi.h"
#include "GraphicsBuffer.hpp"
#include "Common.h"

namespace platform_ex::Windows::D3D12 {
	class Device;
	class CommandContext;

	struct RayTracingGeometrySegement
	{
		GraphicsBuffer* VertexBuffer = nullptr;

		//todo:define direct type
		platform::Render::EFormat VertexFormat =platform::Render::EF_BGR32F;

		// Offset in bytes from the base address of the vertex buffer.
		white::uint32 VertexBufferOffset = 0;

		// Number of bytes between elements of the vertex buffer (sizeof EF_BGR32F by default).
		// Must be equal or greater than the size of the position vector.
		white::uint32 VertexBufferStride = 12;

		// Primitive range for this segment.
		white::uint32 FirstPrimitive = 0;
		white::uint32 NumPrimitives = 0;

		void operator=(const platform::Render::RayTracingGeometrySegement& segment)
		{
			VertexBuffer = static_cast<GraphicsBuffer*>(segment.VertexBuffer);
			VertexFormat = segment.VertexFormat;
			VertexBufferOffset = segment.VertexBufferOffset;
			VertexBufferStride = segment.VertexBufferStride;
			FirstPrimitive = segment.FirstPrimitive;
			NumPrimitives = segment.NumPrimitives;
		}
	};

	/** Bottom level ray tracing acceleration structure (contains triangles). */
	class RayTracingGeometry:public platform::Render::RayTracingGeometry
	{
	public:
		RayTracingGeometry(const platform::Render::RayTracingGeometryInitializer& initializer);

		white::uint32 IndexStride = 0; // 0 for non-indexed / implicit triangle list, 2 for uint16, 4 for uint32
		white::uint32 IndexOffsetInBytes = 0;

		D3D12_RAYTRACING_GEOMETRY_TYPE GeometryType = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

		RayTracingGeometrySegement Segement;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS BuildFlags;


		GraphicsBuffer* IndexBuffer;

		std::shared_ptr<GraphicsBuffer> AccelerationStructureBuffer;
		std::shared_ptr<GraphicsBuffer> ScratchBuffer;

		void BuildAccelerationStructure(platform::Render::CommandContext& CommandContext) override;
		void BuildAccelerationStructure(CommandContext& CommandContext);
	};

	void CreateAccelerationStructureBuffers(std::shared_ptr<GraphicsBuffer>& AccelerationStructureBuffer, std::shared_ptr<GraphicsBuffer>& ScratchBuffer, D3D12Adapter* Deivce, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& PrebuildInfo, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE Type);
}

#endif