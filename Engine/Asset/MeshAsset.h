/*! \file Engine\Asset\MeshAsset.h
\ingroup Engine
\brief Mesh Binary From KlaGE...
*/
#ifndef WE_ASSET_MESH_ASSET_H
#define WE_ASSET_MESH_ASSET_H 1

#include <WBase/sutility.h>
#include <WBase/wmathtype.hpp>

#include "RenderInterface/InputLayout.hpp"

namespace asset {
	class MeshAsset :white::noncopyable {
	public:
		struct SubMeshDescrption
		{
			white::uint8 MaterialIndex;
			struct LodDescription {
				white::uint32 VertexNum;
				white::uint32 VertexBase;
				white::uint32 IndexNum;
				white::uint32 IndexBase;
			};
			std::vector<LodDescription> LodsDescription;
		};

		MeshAsset() = default;

		DefGetter(const wnothrow, const std::vector<platform::Render::Vertex::Element>&, VertexElements, vertex_elements)
			DefGetter(wnothrow, std::vector<platform::Render::Vertex::Element>&, VertexElementsRef, vertex_elements)

			DefGetter(const wnothrow, platform::Render::EFormat, IndexFormat, index_format)
			DefSetter(wnothrow, platform::Render::EFormat, IndexFormat, index_format)

			DefGetter(const wnothrow, white::uint32, VertexCount, vertex_count)
			DefSetter(wnothrow, white::uint32, VertexCount, vertex_count)

			DefGetter(const wnothrow, white::uint32, IndexCount, index_count)
			DefSetter(wnothrow, white::uint32, IndexCount, index_count)

			DefGetter(wnothrow, std::vector<std::unique_ptr<stdex::byte[]>>&, VertexStreamsRef, vertex_streams)
			DefGetter(const wnothrow,const std::vector<std::unique_ptr<stdex::byte[]>>&, VertexStreams, vertex_streams)
	
			DefGetter(wnothrow, std::unique_ptr<stdex::byte[]>&, IndexStreamsRef, index_stream)
			DefGetter(const wnothrow, white::observer_ptr<stdex::byte>, IndexStreams,white::make_observer(index_stream.get()))

			DefGetter(const wnothrow, const std::vector<SubMeshDescrption>&, SubMeshDesces, sub_meshes)
			DefGetter(wnothrow, std::vector<SubMeshDescrption>&, SubMeshDescesRef, sub_meshes)
	private:
		std::vector<platform::Render::Vertex::Element> vertex_elements;
		platform::Render::EFormat index_format;
		std::vector<std::unique_ptr<stdex::byte[]>> vertex_streams;
		std::unique_ptr<stdex::byte[]> index_stream;
		std::vector<SubMeshDescrption> sub_meshes;
		white::uint32 vertex_count;
		white::uint32 index_count;
	};
}


#endif