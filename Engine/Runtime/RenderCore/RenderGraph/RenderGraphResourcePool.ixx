module;

#include "WBase/smart_ptr.hpp"
#include "Core/Math/PlatformMath.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/RenderResource.h"

#include <algorithm>
#include <limits>
#include <vector>

export module RenderGraph:resourcepool;

import "WBase/cassert.h";

import :fwd;
import :definition;
import :resource;

export namespace RenderGraph
{
	class RGBufferPool : public platform::Render::RenderResource
	{
	public:
		RGBufferPool() = default;

		void Tick()
		{
			const uint32 kFramesUntilRelease = 31;

			int32 BufferIndex = 0;
			int32 NumReleasedBuffers = 0;
			int64 NumReleasedBufferBytes = 0;

			while (BufferIndex < AllocatedBuffers.size())
			{
				auto& Buffer = AllocatedBuffers[BufferIndex];

				const bool bIsUnused = Buffer.count() == 1;

				const bool bNotRequestedRecently = (FrameCounter - Buffer->LastUsedFrame) > kFramesUntilRelease;

				if (bIsUnused && bNotRequestedRecently)
				{
					NumReleasedBufferBytes += Buffer->GetAlignDesc().GetSize();

					white::remove_at_swap(AllocatedBuffers, BufferIndex);
					white::remove_at_swap(AllocatedBufferHashes, BufferIndex);

					++NumReleasedBuffers;
				}
				else
				{
					++BufferIndex;
				}
			}

			++FrameCounter;
		}

		white::ref_ptr<RGPooledBuffer> FindFreeBuffer(const RGBufferDesc& Desc, const char* InName, ERGPooledBufferAlignment Alignment = ERGPooledBufferAlignment::Page)
		{
			const uint64 BufferPageSize = 64 * 1024;
			wassume(Desc.BytesPerElement > 0 && Desc.NumElements > 0);
			wassume(Desc.GetSize64() <= std::numeric_limits<uint32>::max());

			auto AlignedDesc = Desc;
			uint64 AlignedNumBytes = Desc.GetSize64();

			switch (Alignment)
			{
			case ERGPooledBufferAlignment::PowerOfTwo:
				AlignedNumBytes = white::math::RoundUpToPowerOfTwo(AlignedNumBytes);
				// Fall through to align up to page size for small buffers; helps with reuse.
				[[fallthrough]];

			case ERGPooledBufferAlignment::Page:
				AlignedNumBytes = white::Align(AlignedNumBytes, BufferPageSize);
				break;

			case ERGPooledBufferAlignment::None:
				break;
			}

			const uint64 AlignedNumElements =
				(AlignedNumBytes + AlignedDesc.BytesPerElement - 1) / AlignedDesc.BytesPerElement;
			if (AlignedNumElements <= std::numeric_limits<uint32>::max() &&
				AlignedNumElements * AlignedDesc.BytesPerElement <= std::numeric_limits<uint32>::max())
			{
				AlignedDesc.NumElements = static_cast<uint32>(AlignedNumElements);
			}
			else
			{
				AlignedDesc = Desc;
			}

			const uint32 BufferHash = AlignedDesc.HashCode();

			// First find if available.
			for (uint64 Index = 0; Index < AllocatedBufferHashes.size(); ++Index)
			{
				if (AllocatedBufferHashes[Index] != BufferHash)
				{
					continue;
				}

				const auto& PooledBuffer = AllocatedBuffers[Index];
				if (PooledBuffer->GetAlignDesc() != AlignedDesc)
				{
					continue;
				}

				// Still being used outside the pool.
				if (PooledBuffer.count() > 1)
				{
					continue;
				}

				PooledBuffer->LastUsedFrame = FrameCounter;
				PooledBuffer->Name = InName;

				// We need the external-facing desc to match what the user requested.
				PooledBuffer->Desc = Desc;

				return PooledBuffer;
			}

			auto& device = platform::Render::Context::Instance().GetDevice();
			auto& CmdList = GetCommandList();
			// Allocate new one
			{
				const uint32 NumBytes = AlignedDesc.GetSize();

				ResourceCreateInfo CreateInfo(InName);
				GraphicsBufferRef BufferRobjcet = device.CreateBuffer(
					platform::Render::Buffer::Usage::MultiFrame,
					Desc.Usage, NumBytes, Desc.BytesPerElement, CreateInfo);

				white::ref_ptr<RGPooledBuffer> PooledBuffer = new RGPooledBuffer(CmdList, std::move(BufferRobjcet), Desc, AlignedDesc.NumElements, InName);
				AllocatedBuffers.emplace_back(PooledBuffer);
				AllocatedBufferHashes.emplace_back(BufferHash);
				wassume(PooledBuffer.count() == 2);

				PooledBuffer->LastUsedFrame = FrameCounter;

				return PooledBuffer;
			}
		}

	private:
		void ReleaseRenderResource() override
		{
			AllocatedBuffers.clear();
			AllocatedBufferHashes.clear();
		}

		std::vector<white::ref_ptr<RGPooledBuffer>> AllocatedBuffers;
		std::vector<uint32> AllocatedBufferHashes;

		uint32 FrameCounter = 0;
	};

	platform::Render::GlobalResource<RGBufferPool> GRenderGraphResourcePool;
}
