/*! \file Engine\WIN32\NTHandle.h
\ingroup Engine
\brief Win32 Kernel HANDLE 接口。
*/

// 这个文件部分代码是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。

#ifndef WE_WIN32_NTHANDLE_H
#define WE_WIN32_NTHANDLE_H 1

#include "Handle.h"

namespace platform_ex {
	namespace Windows {
		namespace Kernel {
			using Handle = void *;

			struct NtHandleCloser {
				wconstexpr Handle operator()() const wnoexcept {
					return nullptr;
				}
				void operator()(Handle hObject) const wnoexcept;
			};
		}

		extern template class MCF::UniqueHandle<Kernel::NtHandleCloser>;

		using UniqueNtHandle = MCF::UniqueHandle<Kernel::NtHandleCloser>;
	}
}


#endif