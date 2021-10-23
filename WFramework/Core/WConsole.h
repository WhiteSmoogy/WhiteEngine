/*!	\file WConsole.h
\ingroup WFrameWork/Core
\brief 控制台。
*/

#ifndef WFrameWork_Core_LConsole_h
#define WFrameWork_Core_LConsole_h 1

namespace white
{
	namespace Consoles
	{
		/*!
		\brief 控制台颜色枚举。
		*/
		enum Color
		{
			Black = 0,
			DarkBlue,
			DarkGreen,
			DarkCyan,
			DarkRed,
			DarkMagenta,
			DarkYellow,
			Gray,
			DarkGray,
			Blue,
			Green,
			Cyan,
			Red,
			Magenta,
			Yellow,
			White
		};

		/*!
		\brief 控制台颜色。
		\note 顺序和 Consoles::Color 对应。
		*/
		/*wconstexpr const ColorSpace::ColorSet ConsoleColors[]{ ColorSpace::Black,
			ColorSpace::Navy, ColorSpace::Green, ColorSpace::Teal, ColorSpace::Maroon,
			ColorSpace::Purple, ColorSpace::Olive, ColorSpace::Silver, ColorSpace::Gray,
			ColorSpace::Blue, ColorSpace::Lime, ColorSpace::Aqua, ColorSpace::Red,
			ColorSpace::Yellow, ColorSpace::Fuchsia, ColorSpace::White };*/
	}
}
#endif