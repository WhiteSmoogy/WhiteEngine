/*!	\file Consoles.h
\ingroup Framework
\ingroup Win32
\brief ����̨��
*/

#ifndef FrameWork_Win32_Consoles_h
#define FrameWork_Win32_Consoles_h 1

#include <WFramework/Win32/WCLib/NLS.h>
#include <cstdlib> // for std::system;
#include <WFramework/WCLib/FContainer.h> // for array;
#include <wincon.h>

namespace platform_ex
{
	namespace Windows
	{
		/*!
		\build Windows ����̨����
		*/
		class WB_API WConsole
		{
		public:
			/*!
			\build Windows ����̨�ı���ʽ��
			*/
			enum Style
			{
				//! brief �����ı�������������
				Normal = 0,
				//! brief �����ı�������������
				Bright = 0x08
			};

		private:
			::HANDLE h_std;
			::WORD saved_attr;

		public:
			::WORD Attributes;

			//! \pre ���ԣ�����ǿ���Ч��
			//@{
			WConsole(unsigned long h = STD_OUTPUT_HANDLE);
			/*!
			\brief ���죺ʹ����֪����̨�����
			*/
			WConsole(::HANDLE);
			//@}
			//! \note �������������ԡ�
			~WConsole();

			//@{
			//! \throw Win32Exception ����ʧ�ܡ�
			//@{
			DefGetter(const, ::COORD, CursorPosition,
				GetScreenBufferInfo().dwCursorPosition)
				::CONSOLE_SCREEN_BUFFER_INFO
				GetScreenBufferInfo() const;
			//@}
			DefGetter(const wnothrow, ::WORD, SavedAttributes, saved_attr)
				//@}

				void
				SetBackColor(std::uint8_t) wnothrow;
			void
				SetCursorPosition(::COORD);
			void
				SetForeColor(std::uint8_t) wnothrow;
			static PDefH(void, SetSystemColor, )
				ImplExpr(std::system("COLOR"))
				static void
				SetSystemColor(std::uint8_t);

			static PDefH(void, Clear, )
				ImplExpr(std::system("CLS"))

				static ::WORD
				ComposeAttributes(std::uint8_t, std::uint8_t) wnothrow;

			//@{
			void
				CursorUp(size_t);

			static PDefH(array<std::uint8_t WPP_Comma 2>, DecomposeAttributes,
				::WORD val) wnothrow
				ImplRet({ { FetchForeColor(val), FetchBackColor(val) } })

				//! \throw Win32Exception ����ʧ�ܡ�
				//@{
				void
				Erase(wchar_t = L' ');

			static PDefH(std::uint8_t, FetchBackColor, ::WORD val) wnothrow
				ImplRet(std::uint8_t((val >> 4) & 15))

				static PDefH(std::uint8_t, FetchForeColor, ::WORD val) wnothrow
				ImplRet(std::uint8_t(val & 15))

				void
				Fill(::COORD, unsigned long, wchar_t = L' ');

			/*!
			\brief �������ԡ�
			*/
			void
				RestoreAttributes();

			void
				Update();
			void
				Update(::WORD);

			void
				UpdateBackColor(std::uint8_t);

			void
				UpdateForeColor(std::uint8_t);
			//@}

			/*!
			\brief ����ַ�����
			\pre ��һ����������ָ��ǿա�
			\return ������ַ�����
			*/
			//@{
			size_t
				WriteString(string_view);
			//! \note �ڶ�����Ϊ����ҳ��
			size_t
				WriteString(string_view, unsigned);
			size_t
				WriteString(wstring_view);
			PDefH(size_t, WriteString, u16string_view sv)
				ImplRet(WriteString({ platform::wcast(sv.data()), sv.length() }))
				//@}
				//@}
		};

		/*!
		\ingroup helper_functions
		\brief ��������̨����
		\return ָ���´����Ŀ���̨�����ָ�룬��ʧ����Ϊ�ա�
		\note ���׳� Win32Exception ��
		\relates WConsole
		*/
		WB_API unique_ptr<WConsole>
			MakeWConsole(unsigned long = STD_OUTPUT_HANDLE);

	} // inline namespace Windows;

} // namespace platform_ex;

#endif