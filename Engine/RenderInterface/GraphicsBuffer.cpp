#include "IGraphicsBuffer.hpp"

platform::Render::GraphicsBuffer::GraphicsBuffer(Buffer::Usage usage_, white::uint32 access_, white::uint32 size_in_byte_)
	:usage(usage_),access(access_), size_in_byte(size_in_byte_)
{
}

platform::Render::GraphicsBuffer::~GraphicsBuffer() {
}