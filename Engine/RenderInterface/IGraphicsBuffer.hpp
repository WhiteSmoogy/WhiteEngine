/*! \file Engine\Render\IGraphicsBuffer.hpp
\ingroup Engine
\brief Buffer½Ó¿ÚÀà¡£
*/
#ifndef WE_RENDER_IGraphicsBuffer_hpp
#define WE_RENDER_IGraphicsBuffer_hpp 1

#include <WBase/winttype.hpp>
#include <WBase/sutility.h>
#include <WBase/wmacro.h>
#include <WBase/span.hpp>
#include "RenderObject.h"
#include "IGPUResourceView.h"

namespace platform::Render {
	class CommandListBase;
	class CommandList;
	class CommandListImmediate;

	namespace Buffer {
		enum  Usage
		{
			Static = 0x0001,
			Dynamic = 0x0002,

			SingleDraw = 0x8100,
			SingleFrame = 0x8200,
			MultiFrame = 0x8300,

			LifeTimeMask = 0x8F00,
		};

		enum Access {
			Read_Only,
			Write_Only,
			Read_Write,
			Write_No_Overwrite
		};

		class Mapper;
	}

	struct BufferDesc
	{
		white::uint32 Size = 0;
		white::uint32 Stride = 0;
		Buffer::Usage Usage{};
		white::uint32 Access{};

		bool IsDynamic() const
		{
			return (Usage & Buffer::Usage::Dynamic) ? true : false;;
		}
	};

	class GraphicsBuffer:public RObject {
	public:
		virtual ~GraphicsBuffer();


		DefGetter(const wnothrow, white::uint32, Size, size_in_byte)

			DefGetter(const wnothrow, Buffer::Usage, Usage, usage)

			DefGetter(const wnothrow, white::uint32, Access, access)


			virtual void CopyToBuffer(GraphicsBuffer& rhs) = 0;

		virtual void UpdateSubresource(white::uint32 offset, white::uint32 size, void const * data) = 0;
	protected:
		GraphicsBuffer(Buffer::Usage usage, white::uint32 access, white::uint32 size_in_byte);

	private:
		virtual void* Map(CommandListImmediate& CmdList,Buffer::Access ba) = 0;
		virtual void Unmap(CommandListImmediate& CmdList) = 0;

		friend class Buffer::Mapper;
	protected:
		Buffer::Usage usage;
		white::uint32 access;

		white::uint32 size_in_byte;
	};

	using GraphicsBufferRef = white::ref_ptr<GraphicsBuffer, RObjectController>;

	namespace Buffer {
		class Mapper : white::noncopyable
		{
			friend class ::platform::Render::GraphicsBuffer;

		public:
			Mapper(GraphicsBuffer& InBuffer, Access ba);

			~Mapper()
			{
				buffer.Unmap(CmdList);
			}

			template <typename T>
			const T* Pointer() const
			{
				return static_cast<T*>(data);
			}
			template <typename T>
			T* Pointer()
			{
				return static_cast<T*>(data);
			}

		private:
			CommandListImmediate& CmdList;

			GraphicsBuffer& buffer;
			void* data;
		};
	}

	class ConstantBuffer :public RObject {
	public:
		virtual void Update(platform::Render::CommandList& CmdList,white::uint32 size, void const* data) = 0;
	};

	GraphicsBuffer* CreateVertexBuffer(white::span<const std::byte> Contents, Buffer::Usage Usage,white::uint32 Access);

	GraphicsBuffer* CreateBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, uint32 stride, ResourceCreateInfo init_data = {});

	struct BufferUAVCreateInfo
	{
		EFormat Format = EFormat::EF_Unknown;
		bool bSupportsAtomicCounter = false;
		bool bSupportsAppendBuffer = false;
	};

	struct BufferSRVCreateInfo
	{
		EFormat Format = EFormat::EF_Unknown;

		uint32 StartOffsetBytes = 0;

		/** Number of elements (whole buffer by default) */
		uint32 NumElements = std::numeric_limits<uint32>::max();
	};

	class BufferViewCache
	{
	public:
		// Finds a UAV matching the descriptor in the cache or creates a new one and updates the cache.
		UnorderedAccessView* GetOrCreateUAV(CommandListBase& RHICmdList, GraphicsBuffer* Buffer, const BufferUAVCreateInfo& CreateInfo);

		// Finds a SRV matching the descriptor in the cache or creates a new one and updates the cache.
		ShaderResourceView* GetOrCreateSRV(CommandListBase& RHICmdList, GraphicsBuffer* Buffer, const BufferSRVCreateInfo& CreateInfo);

	private:
		std::vector<std::pair<BufferUAVCreateInfo, UAVRIRef>> UAVs;
		std::vector<std::pair<BufferSRVCreateInfo, SRVRIRef>> SRVs;
	};
}

#endif