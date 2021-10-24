// ����ļ����ִ����� MCF ��һ���֡�
// �йؾ�����Ȩ˵��������� MCFLicense.txt��

#include "NTHandle.h"
#include <WFramework/WCLib/Debug.h>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>


extern "C" {

	typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

	NTSTATUS(WINAPI * NTCLOSE)(HANDLE);

	auto NtClose = reinterpret_cast<decltype(NTCLOSE)>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtClose"));

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif
}

namespace platform_ex {
	namespace Windows {
		template class MCF::UniqueHandle<Kernel::NtHandleCloser>;

		namespace Kernel {
			void NtHandleCloser::operator()(Handle hObject) const noexcept {
				const auto lStatus = ::NtClose(hObject);
				WAssert(NT_SUCCESS(lStatus), "NtClose() ʧ�ܡ�");
			}
		}
	}
}