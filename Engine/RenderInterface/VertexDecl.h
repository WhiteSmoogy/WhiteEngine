#pragma once

#include "WBase/wmacro.h"
#include "IFormat.hpp"
#include <vector>

namespace platform::Render {
	namespace Vertex {
		enum Usage {
			Position,//位置

			Normal,//法线
			Tangent,
			Binoraml,

			Diffuse,//顶点色
			Specular,//顶点高光
			BlendWeight,
			BlendIndex,

			TextureCoord,

		};

		struct Element {
			Element() = default;

			Element(Usage usage_, white::uint8 index, EFormat format_)
				:usage(usage_), usage_index(index), format(format_)
			{}

			DefGetter(const wnothrow, uint16, ElementSize, NumFormatBytes(format))

				friend bool operator==(const Element& lhs, const Element& rhs) {
				return lhs.format == rhs.format &&
					lhs.usage == rhs.usage &&
					lhs.usage_index == rhs.usage_index;
			}

			Usage usage;
			white::uint8 usage_index;
			EFormat format;
		};
	}

	enum class PrimtivteType
	{
		TriangleList,
		TriangleStrip,
		LineList,
		PointList,

		ControlPoint_0,
		ControlPoint_1 = ControlPoint_0 + 1,
		ControlPoint_32 = ControlPoint_0 + 32,
	};

	inline uint32 GetPrimitiveTypeFactor(PrimtivteType type)
	{
		switch (type)
		{
		case platform::Render::PrimtivteType::TriangleList:
			return 3;
		case platform::Render::PrimtivteType::TriangleStrip:
			return 1;
		case platform::Render::PrimtivteType::LineList:
			return 2;
		case platform::Render::PrimtivteType::PointList:
			return 1;
		default:
			break;
		}

		return type >= PrimtivteType::ControlPoint_1 ? (uint32)type - (uint32)PrimtivteType::ControlPoint_0 : 1;
	}

	struct VertexElement
	{
		EFormat Format;
		Vertex::Usage Usage;
		uint16 Stride;
		uint8 StreamIndex;
		uint8 Offset;
		white::uint8 UsageIndex;

		friend bool operator==(const VertexElement&, const VertexElement&) = default;
	};

	constexpr inline VertexElement CtorVertexElement(
		uint8 StreamIndex,
		uint8 Offset,
		Vertex::Usage Usage,
		white::uint8 UsageIndex,
		EFormat Format,
		uint16 Stride
	)
	{
		VertexElement ve{};

		ve.StreamIndex = StreamIndex;
		ve.Offset = Offset;
		ve.Usage = Usage;
		ve.UsageIndex = UsageIndex;
		ve.Format = Format;
		ve.Stride = Stride;

		return ve;
	}

	using VertexDeclarationElements = std::vector<VertexElement>;

	constexpr unsigned MaxVertexElementCount = 16;
}