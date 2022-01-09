/*! \file Render\ViewPort.h
\ingroup Engine
\brief 视口结构。
*/
#ifndef WE_RENDER_VIEWPORT_H
#define WE_RENDER_VIEWPORT_H 1

#include <WBase/sutility.h>
#include <WBase/winttype.hpp>

namespace WhiteEngine::Render {
	struct ViewPort
	{
		ViewPort() = default;

		int x;
		int y;
		white::uint16 width;
		white::uint16 height;

		float zmin = 0;
		float zmax = 1;

		ViewPort(int _x, int _y, white::uint16 _width, white::uint16 _height)
			:x(_x), y(_y),
			width(_width),
			height(_height),
			zmin(0),
			zmax(1)
		{
		}
	};
}

#endif