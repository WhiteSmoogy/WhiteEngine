/*! \file Engine\WIN32\File.h
\ingroup Engine
\brief Win32 File �ӿڡ�
*/
#ifndef WE_WIN32_File_H
#define WE_WIN32_File_H 1

#include <WBase/winttype.hpp>

#ifdef WB_IMPL_MSCPP
#include <string_view>
#else
#include <experimental/string_view> //TODO:replace it
#endif
#include "NTHandle.h"

namespace platform_ex {
	namespace Windows {
		using namespace white::inttype;

		// ������벿���� MCF ��һ���֡�
		// �йؾ�����Ȩ˵��������� MCFLicense.txt��
		class File {
		public:
			enum : uint32 {
				// Ȩ�޿��ơ�
				kToRead = 0x00000001,
				kToWrite = 0x00000002,

				// ������Ϊ���ơ����û��ָ�� TO_WRITE����Щѡ���Ч��
				kDontCreate = 0x00000004, // �ļ���������ʧ�ܡ���ָ����ѡ���� kFailIfExists ��Ч��
				kFailIfExists = 0x00000008, // �ļ��Ѵ�����ʧ�ܡ�

				// �������Ȩ�ޡ�
				kSharedRead = 0x00000100, // �����Ȩ�ޡ����ڲ��� kToWrite �򿪵��ļ����ǿ����ġ�
				kSharedWrite = 0x00000200, // ����дȨ�ޡ�
				kSharedDelete = 0x00000400, // ����ɾ��Ȩ�ޡ�

				// ���
				kNoBuffering = 0x00010000,
				kWriteThrough = 0x00020000,
				kDeleteOnClose = 0x00040000,
				kDontTruncate = 0x00080000, // Ĭ�������ʹ�� kToWrite ���ļ�������������ݡ�
			};

#ifdef WB_IMPL_MSCPP
			using wstring_view = std::wstring_view;
#else
			using wstring_view = std::experimental::wstring_view;
#endif
		private:
			static UniqueNtHandle CreateFileWA(const wstring_view& path, uint32 flags);
		private:
			UniqueNtHandle file;

		public:
			wconstexpr File() wnoexcept
				: file()
			{
			}
			File(const wstring_view& path, uint32 flags)
				: file(CreateFileWA(path, flags))
			{
			}

		public:
			void *GetHandle() const wnoexcept {
				return file.Get();
			}

			bool IsOpen() const wnoexcept;
			void Open(const wstring_view& path, uint32 flags);
			bool OpenNothrow(const wstring_view& path, uint32 flags);
			void Close() wnoexcept;

			uint64 GetSize() const;
			void Resize(uint64 u64NewSize);
			void Clear();

			// 1. fnAsyncProc ���ǻᱻִ��һ�Σ���ʹ��ȡ��д�����ʧ�ܣ�
			// 2. ���еĻص������������׳��쳣������������£��쳣���ڶ�ȡ��д�������ɻ�ʧ�ܺ������׳���
			// 3. ���ҽ��� fnAsyncProc �ɹ��������첽�����ɹ��� fnCompletionCallback �Żᱻִ�С�
			std::size_t Read(void *pBuffer, std::size_t uBytesToRead, uint64 u64Offset/*,
				FunctionView<void()> fnAsyncProc = nullptr, FunctionView<void()> fnCompletionCallback = nullptr*/) const;
			std::size_t Write(uint64 u64Offset, const void *pBuffer, std::size_t uBytesToWrite/*,
				FunctionView<void()> fnAsyncProc = nullptr, FunctionView<void()> fnCompletionCallback = nullptr*/);
			void HardFlush();

			void Swap(File &rhs) wnoexcept {
				using std::swap;
				swap(file, rhs.file);
			}

		public:
			explicit operator bool() const wnoexcept {
				return IsOpen();
			}

			friend void swap(File &lhs, File &rhs) wnoexcept {
				lhs.Swap(rhs);
			}
		};
	}
}


#endif
