#pragma once

#include <WBase/wdef.h>
#include <Engine/emacro.h>
#include <WBase/memory.hpp>

#include "Allocation.h"
#include "RenderInterface/IGraphicsBuffer.hpp"
#include "Common.h"

#include <algorithm>

namespace platform_ex::Windows::D3D12
{
	constexpr unsigned MAX_GLOBAL_CONSTANT_BUFFER_SIZE = 4096;

	class FastConstantBuffer : public DeviceChild
	{
	public:
		FastConstantBuffer(NodeDevice* InParent, FastConstantAllocator& Allocator);

		/**
		* Updates a variable in the constant buffer.
		* @param Data - The data to copy into the constant buffer
		* @param Offset - The offset in the constant buffer to place the data at
		* @param InSize - The size of the data being copied
		*/
		void UpdateConstant(const uint8* Data, uint16 Offset, uint16 InSize)
		{
			// Check that the data we are shadowing fits in the allocated shadow data
			wconstraint((uint32)Offset + (uint32)InSize <= MAX_GLOBAL_CONSTANT_BUFFER_SIZE);
			std::memcpy(ShadowData + Offset, Data, InSize);
			CurrentUpdateSize = std::max((uint32)(Offset + InSize), CurrentUpdateSize);

			bIsDirty = true;
		}

		void Reset() { CurrentUpdateSize = 0; }

		bool Version(ResourceLocation& BufferOut, bool bDiscardSharedConstants);
	protected:
		__declspec(align(16)) uint8 ShadowData[MAX_GLOBAL_CONSTANT_BUFFER_SIZE];

		/** Size of all constants that has been updated since the last call to Commit. */
		uint32	CurrentUpdateSize;

		/**
		 * Size of all constants that has been updated since the last Discard.
		 * Includes "shared" constants that don't necessarily gets updated between every Commit.
		 */
		uint32	TotalUpdateSize;

		// Indicates that a constant has been updated but this one hasn't been flushed.
		bool bIsDirty;
	private:
		FastConstantAllocator& Allocator;
	};

	class ConstantBuffer :public platform::Render::ConstantBuffer, public DeviceChild
	{
	public:
		using UsageType = platform::Render::Buffer::Usage;

		ResourceLocation Location;

		const UsageType Usage;

		const uint32 ConstantBufferSize;

		ConstantBuffer(NodeDevice* InParent, UsageType InUsage, uint32 InConstantBufferSize)
			:DeviceChild(InParent), Location(InParent), Usage(InUsage), ConstantBufferSize(InConstantBufferSize)
		{}

		void Update(platform::Render::CommandList& CmdList, white::uint32 size, void const* data) final;
	};

}