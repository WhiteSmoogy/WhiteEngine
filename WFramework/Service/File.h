/*!	\file File.h
\ingroup Service
\brief 平台中立的文件抽象。
*/


#ifndef Framework_Service_File_h_
#define Framework_Service_File_h_ 1

#include <WFramework/Core/WCoreUtilities.h>
//	ofstream;
#include <WBase/streambuf.hpp> // for white::membuf;

namespace white
{
	namespace IO
	{

		/*!
		\brief 打开文件。
		\pre 路径参数非空。
		\throw std::system_error 打开失败。
		\return 非空的文件指针。
		\sa uopen
		*/
		WF_API WB_NONNULL(1) UniqueFile
			OpenFile(const char*, int, mode_t = 0);


		/*!
		\brief 移除文件链接。
		\pre 间接断言：参数非空。
		\throw std::system_error 文件存在且操作失败。
		*/
		//@{
		template<typename _tChar>
		WB_NONNULL(1) void
			Remove(const _tChar* path)
		{
			if WB_UNLIKELY(!uremove(path))
			{
				const int err(errno);

				if (err != ENOENT)
					white::throw_error(err, "Failed removing destination file '"
						+ IO::MakePathString(path) + '\'');
			}
		}

		template<typename _tChar>
		WB_NONNULL(1) void
			Unlink(const _tChar* path)
		{
			if WB_UNLIKELY(!uunlink(path))
			{
				const int err(errno);

				if (err != ENOENT)
					white::throw_error(err, "Failed unlinking destination file '"
						+ IO::MakePathString(path) + '\'');
			}
		}
		//@}


		//@{
		//! \brief 共享锁定的文件映射输入流。
		class WF_API SharedInputMappedFileStream : private MappedFile,
			private SharedIndirectLockGuard<const UniqueFile>, private white::membuf,
			public std::istream
		{
		public:
			WB_NONNULL(1)
				SharedInputMappedFileStream(const char*);

			explicit DefCvt(const wnothrow, bool, !fail())

				//! \brief 虚析构：类定义外默认实现。
				~SharedInputMappedFileStream() override;
		};


		//! \brief 独占锁定的文件输出流。
		class WF_API UniqueLockedOutputFileStream : public ofstream
		{
		private:
			FileDescriptor desc;
			unique_lock<FileDescriptor> lock;

			UniqueLockedOutputFileStream(int);

		public:
			UniqueLockedOutputFileStream(UniqueFile);
			//! \pre 间接断言：指针参数非空。
			//@{
			template<typename _tChar>
			WB_NONNULL(1)
				UniqueLockedOutputFileStream(const _tChar* filename, int omode,
					mode_t pmode = DefaultPMode())
				: UniqueLockedOutputFileStream(uopen(filename, omode, pmode))
			{}
			/*!
			\note std::ios_base::openmode 可能是 int 。
			*/
			template<typename _tChar>
			WB_NONNULL(1)
				UniqueLockedOutputFileStream(std::ios_base::openmode mode,
					const _tChar* filename, mode_t pmode = DefaultPMode())
				: UniqueLockedOutputFileStream(filename, omode_conv(mode), pmode)
			{}
			//@}

			//! \brief 虚析构：类定义外默认实现。
			~UniqueLockedOutputFileStream() override;
		};
		//@}

	} // namespace IO;

} // namespace white;

#endif

