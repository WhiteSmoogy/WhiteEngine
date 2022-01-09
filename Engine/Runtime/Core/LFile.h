/*! \file Core\LFile.h
\ingroup Engine
\brief �ļ���ȡ/д�� �ӿڡ�
\brief ����ϵͳ����
*/
#ifndef WE_Core_File_H
#define WE_Core_File_H 1

#include <WFramework/WCLib/Platform.h>

#ifdef WFL_Win32
#include "System/Win32/File.h"
#include <WBase/winttype.hpp>

namespace platform {
	using platform_ex::Windows::File;
	using namespace white::inttype;

	class FileRead {
	public:
		FileRead(File const & file_)
			:file(file_)
		{}

		std::size_t Read(void* pBuffer, std::size_t uBytesToRead) {
			auto result = file.Read(pBuffer, uBytesToRead, u64Offset);
			u64Offset += result;
			return result;
		}

		void SkipTo(std::size_t uBytesTarget) {
			u64Offset = uBytesTarget;
		}

		void Skip(std::size_t uBytesOffset) {
			u64Offset += uBytesOffset;
		}

		uint64 GetOffset() const {
			return u64Offset;
		}

	private:
		File const & file;
		uint64 u64Offset = 0;
	};
}

#endif




#endif