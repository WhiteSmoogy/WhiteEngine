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
#include "ShaderParametersMetadata.h"
namespace platform::Render {
	class CommandList;

	namespace Buffer {
		enum  Usage
		{
			Static = 0x0001,
			Dynamic = 0x0002,

			AccelerationStructure = 0x8000,

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

	class GraphicsBuffer:public RObject {
	public:
		virtual ~GraphicsBuffer();


		DefGetter(const wnothrow, white::uint32, Size, size_in_byte)

			DefGetter(const wnothrow, Buffer::Usage, Usage, usage)

			DefGetter(const wnothrow, white::uint32, Access, access)


			virtual void CopyToBuffer(GraphicsBuffer& rhs) = 0;

		virtual void HWResourceCreate(void const * init_data) = 0;
		virtual void HWResourceDelete() = 0;

		virtual void UpdateSubresource(white::uint32 offset, white::uint32 size, void const * data) = 0;
	protected:
		GraphicsBuffer(Buffer::Usage usage, white::uint32 access, white::uint32 size_in_byte);

	private:
		virtual void* Map(Buffer::Access ba) = 0;
		virtual void Unmap() = 0;

		friend class Buffer::Mapper;
	protected:
		Buffer::Usage usage;
		white::uint32 access;

		white::uint32 size_in_byte;
	};

	namespace Buffer {
		class Mapper : white::noncopyable
		{
			friend class ::platform::Render::GraphicsBuffer;

		public:
			Mapper(GraphicsBuffer& InBuffer, Access ba)
				: buffer(InBuffer)
			{
				data = buffer.Map(ba);
			}
			~Mapper()
			{
				buffer.Unmap();
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
			GraphicsBuffer& buffer;
			void* data;
		};
	}

	class ConstantBuffer :public RObject {
	public:
		virtual void Update(platform::Render::CommandList& CmdList,white::uint32 size, void const* data) = 0;
	};

	ConstantBuffer* CreateConstantBuffer(const void* Contents, Buffer::Usage Usage,const ShaderParametersMetadata& Layout);

	template<typename TBufferStruct>
	requires requires{ TBufferStruct::TypeInfo::GetStructMetadata(); }
	class GraphicsBufferRef
	{
	public:
		static GraphicsBufferRef<TBufferStruct> CreateGraphicsBuffeImmediate(const TBufferStruct& Value, Buffer::Usage Usage)
		{
			return GraphicsBufferRef<TBufferStruct>(CreateConstantBuffer(&Value, Usage, *TBufferStruct::TypeInfo::GetStructMetadata()));
		}

		operator std::shared_ptr<ConstantBuffer>()
		{
			return buffer;
		}

		ConstantBuffer* Get() const
		{
			return buffer.get();
		}
	private:
		GraphicsBufferRef(ConstantBuffer* InBuffer)
			:buffer(InBuffer,RObjectDeleter())
		{
		}

		std::shared_ptr<ConstantBuffer> buffer;
	};

	template<typename TBufferStruct>
	requires requires{ TBufferStruct::TypeInfo::GetStructMetadata(); }
	GraphicsBufferRef<TBufferStruct> CreateGraphicsBuffeImmediate(const TBufferStruct& Value, Buffer::Usage Usage)
	{
		return GraphicsBufferRef<TBufferStruct>::CreateGraphicsBuffeImmediate(Value, Usage);
	}

	GraphicsBuffer* CreateVertexBuffer(white::span<const std::byte> Contents, Buffer::Usage Usage,uint32 Access);
}

#endif