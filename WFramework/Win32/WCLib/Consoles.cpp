#include "../../WCLib/Platform.h"
#if WFL_Win32
#include "Consoles.h" // for UTF8ToWCS;
#include "../../Core/WCoreUtilities.h"
using namespace white;
#endif

namespace platform_ex
{
#if WFL_Win32

	namespace Windows
	{
		WConsole::WConsole(unsigned long dev)
			: WConsole(::GetStdHandle(dev))
		{}
		WConsole::WConsole(::HANDLE h)
			: h_std(h), saved_attr(GetScreenBufferInfo().wAttributes),
			Attributes(saved_attr)
		{
			WAssert(h_std && h_std != INVALID_HANDLE_VALUE, "Invalid handle found.");
		}
		WConsole::~WConsole()
		{
			FilterExceptions(std::bind(&WConsole::RestoreAttributes, this), wfsig);
		}

		::CONSOLE_SCREEN_BUFFER_INFO
			WConsole::GetScreenBufferInfo() const
		{
			::CONSOLE_SCREEN_BUFFER_INFO info;

			WCL_CallF_Win32(GetConsoleScreenBufferInfo, h_std, &info);
			return info;
		}

		void
			WConsole::SetSystemColor(std::uint8_t color)
		{
			char cmd[9];

			std::sprintf(cmd, "COLOR %02x", int(color));
			std::system(cmd);
		}

		void
			WConsole::SetBackColor(std::uint8_t bc) wnothrow
		{
			Attributes = ComposeAttributes(FetchForeColor(Attributes), bc);
		}
		void
			WConsole::SetCursorPosition(::COORD pos)
		{
			// NOTE: %::SetConsoleCursorPosition expects 1-based.
			WCL_CallF_Win32(SetConsoleCursorPosition, h_std, { short(pos.X + 1), short(pos.Y + 1) });
		}
		void
			WConsole::SetForeColor(std::uint8_t fc) wnothrow
		{
			Attributes = ComposeAttributes(fc, FetchBackColor(Attributes));
		}

		::WORD
			WConsole::ComposeAttributes(std::uint8_t fore, std::uint8_t back) wnothrow
		{
			return (fore & 15) | ((back & 15) << 4);
		}

		void
			WConsole::CursorUp(size_t num_rows)
		{
			if (num_rows != 0)
			{
				const auto pos(GetCursorPosition());

				// XXX: Conversion to 'short' might be implementation-defined.
				SetCursorPosition({ short(pos.Y - short(num_rows)), pos.X });
			}
		}

		void
			WConsole::Erase(wchar_t c)
		{
			const auto size(GetScreenBufferInfo().dwSize);

			Fill({ short(), short() }, CheckPositiveScalar<unsigned long>(size.X)
				* CheckPositiveScalar<unsigned long>(size.Y), c);
		}

		void
			WConsole::Fill(::COORD coord, unsigned long n, wchar_t c)
		{
			WCL_CallF_Win32(FillConsoleOutputCharacterW, h_std, c, n, coord, {});
			WCL_CallF_Win32(FillConsoleOutputAttribute, h_std, Attributes, n, coord, {});
			WCL_CallF_Win32(SetConsoleCursorPosition, h_std, { coord.X, coord.Y });
		}

		void
			WConsole::RestoreAttributes()
		{
			//	SetColor();
			Update(GetSavedAttributes());
		}

		void
			WConsole::Update()
		{
			Update(Attributes);
		}
		void
			WConsole::Update(::WORD value)
		{
			WCL_CallF_Win32(SetConsoleTextAttribute, h_std, value);
		}

		void
			WConsole::UpdateBackColor(std::uint8_t fc)
		{
			SetBackColor(fc);
			Update();
		}

		void
			WConsole::UpdateForeColor(std::uint8_t fc)
		{
			SetForeColor(fc);
			Update();
		}

		size_t
			WConsole::WriteString(string_view sv)
		{
			WAssertNonnull(sv.data());
			// FIXME: Support for non-BMP characters.
			return WriteString(UTF8ToWCS(sv));
		}
		size_t
			WConsole::WriteString(string_view sv, unsigned cp)
		{
			WAssertNonnull(sv.data());
			return WriteString(MBCSToWCS(sv, cp));
		}
		size_t
			WConsole::WriteString(wstring_view sv)
		{
			WAssertNonnull(sv.data());

			unsigned long n;

			WCL_CallF_Win32(WriteConsoleW, h_std, sv.data(),
				static_cast<unsigned long>(sv.length()), &n, wimpl({}));
			return size_t(n);
		}

		unique_ptr<WConsole>
			MakeWConsole(unsigned long h)
		{
			unique_ptr<WConsole> p_con;

			TryExpr(p_con.reset(new WConsole(h)))
				CatchIgnore(Win32Exception&)
				return p_con;
		}

	} // inline namespace Windows;

#endif

} // namespace platform_ex;


