/*! \file Engine\Render\InputLayout.hpp
\ingroup Engine\Render
\输入的布局信息 包括 顶点流布局信息 索引信息
*/
#pragma once
#ifndef WE_RENDER_INPUTLATOYT_HPP
#define WE_RENDER_INPUTLATOYT_HPP 1

#include <WBase/winttype.hpp>
#include <WBase/memory.hpp>
#include "IGraphicsBuffer.hpp"
#include "VertexDecl.h"

namespace platform::Render {
	namespace Vertex {
		struct Stream {
			enum Type {
				Geomerty,
				//Instance,
			} type;

			std::shared_ptr<GraphicsBuffer> stream;
			std::vector<Element> elements;
			uint32 vertex_size;

			white::uint32 instance_freq;
		};

		enum InputLayoutFormat :uint8 {
			Empty,

			P3F_N3F_T2F	= 1, //!1.4 
			P3F_N4B_T2F,	//!TODO
		};
	}

	//fixme:error class name
	class InputLayout :public white::noncopyable {
	public:
		InputLayout();
		virtual ~InputLayout();

		DefGetter(const wnothrow, PrimtivteType, TopoType, topology_type)
			DefSetter(, PrimtivteType, TopoType, topology_type)

		uint32 GetNumVertices() const wnothrow;

		void BindVertexStream(const std::shared_ptr<GraphicsBuffer> & buffer, std::initializer_list<Vertex::Element> elements, Vertex::Stream::Type type = Vertex::Stream::Geomerty, uint32 instance_freq = 1);

		DefGetter(const wnothrow, uint32, VertexStreamsSize, static_cast<uint32>(vertex_streams.size()))


		const Vertex::Stream& GetVertexStream(uint32 index) const;

		Vertex::Stream& GetVertexStreamRef(uint32 index);

		void BindIndexStream(const std::shared_ptr<GraphicsBuffer> & buffer, EFormat format);

		DefGetter(const wnothrow, const std::shared_ptr<GraphicsBuffer>&, IndexStream, index_stream)
		DefGetter(const wnothrow,EFormat,IndexFormat,index_format)

		uint32 GetNumIndices() const;


		DefGetter(const wnothrow, uint32, VertexStart, start_vertex_location)
		DefGetter(wnothrow, uint32&, VertexStartRef, start_vertex_location)

		DefGetter(const wnothrow, uint32, IndexStart, start_index_location)
		DefGetter(wnothrow, uint32&, IndexStartRef, start_index_location)

		VertexDeclarationElements GetVertexDeclaration() const;

	protected:
		void Dirty();

	protected:
		PrimtivteType topology_type;

		std::vector<Vertex::Stream> vertex_streams;
		//Vertex::Stream instance_stream;

		mutable VertexDeclarationElements vertex_decl;

		std::shared_ptr<GraphicsBuffer> index_stream;
		EFormat index_format;

		uint32 start_vertex_location;
		uint32 start_index_location;

		//int32 base_vertex_location;
		//uint32 start_instance_location;

		bool stream_dirty;
	};
}

#endif