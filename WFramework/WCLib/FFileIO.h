/*!	\file FFileIO.h
\ingroup Framework
\brief ����������������������
*/

#ifndef FrameWork_FFileIO_h
#define FrameWork_FFileIO_h 1

#include <WFramework/WCLib/Platform.h>
#include <WBase/wmacro.h>
#include <WBase/string.hpp>
#include <WFramework/WCLib/Debug.h>
#include <WFramework/WCLib/FReference.h>
#include <WFramework/WCLib/FContainer.h>
#include <WBase/container.hpp>

#include <chrono>
#include <ios>
#include <fstream>
#include <system_error>

#if __GLIBCXX__
#include <ext/stdio_filebuf.h>
#include <WBase/utility.hpp>
#endif

namespace platform
{
	/*!
	\ingroup diagnostic
	\brief �����Ϣ�ͺ���ǩ���ַ�����
	\pre ��Ӷ��ԣ�ָ������ǿա�
	\note ʹ�� ADL to_string ��
	*/
	//@{
	inline WB_NONNULL(2) PDefH(string, ComposeMessageWithSignature,
		const string& msg, const char* sig)
		ImplRet(msg + " @ " + Nonnull(sig))
		inline WB_NONNULL(1, 2) PDefH(string, ComposeMessageWithSignature,
			const char* msg, const char* sig)
		ImplRet(string(Nonnull(msg)) + " @ " + Nonnull(sig))
		template<class _type>
	inline WB_NONNULL(2) string
		ComposeMessageWithSignature(const _type& msg, const char* sig)
	{
		using white::to_string;

		return to_string(msg) + " @ " + Nonnull(sig);
	}
	//@}

	/*!
	\brief �����ʺϱ�ʾ·���� \c char �ַ�����
	\note �ַ����ͷ� \c char ʱת����
	*/
	//@{
	//! \pre ��Ӷ��ԣ������ǿա�
	inline WB_NONNULL(1) PDefH(string, MakePathString, const char* s)
		ImplRet(Nonnull(s))
		inline PDefH(const string&, MakePathString, const string& s)
		ImplRet(s)
		//! \pre Win32 ƽ̨����ʵ�ֲ�ֱ�ӷ�����ֵ���ַ��Ķ�̬���Ϳ�Ϊ���ּ��ݵ��������͡�
		//@{
		//! \pre ��Ӷ��ԣ������ǿա�
		WF_API WB_NONNULL(1) string
		MakePathString(const char16_t*);
	inline PDefH(string, MakePathString, u16string_view sv)
		ImplRet(MakePathString(sv.data()))
		//@}
		//@}


		//@{
		//! \brief �ļ��ڵ��ʶ���͡�
		//@{
#if WFL_Win32
		using FileNodeID = pair<std::uint32_t, std::uint64_t>;
#else
		using FileNodeID = pair<std::uint64_t, std::uint64_t>;
#endif
	//@}

	/*!
	\bug �ṹ��������Ⱦ��
	\relates FileNodeID
	*/
	//@{
	wconstfn PDefHOp(bool, == , const FileNodeID& x, const FileNodeID& y)
		ImplRet(x.first == y.first && x.second == y.second)
		wconstfn PDefHOp(bool, != , const FileNodeID& x, const FileNodeID& y)
		ImplRet(!(x == y))
		//@}
		//@}


		using FileTime = std::chrono::nanoseconds;


	/*!
	\brief �ļ�ģʽ���͡�
	*/
	//@{
#if WFL_Win32
	using mode_t = unsigned short;
#else
	using mode_t = ::mode_t;
#endif
	//@}


	/*!
	\brief �ļ��ڵ����
	*/
	enum class NodeCategory : std::uint_least32_t;

	/*
	\brief ȡָ��ģʽ��Ӧ���ļ��ڵ����͡�
	\relates NodeCategory
	\since build 658
	*/
	WF_API NodeCategory
		CategorizeNode(mode_t) wnothrow;


	/*!
	\brief �ļ���������װ�ࡣ
	\note ��������Լ�����������쳣�׳���֤�Ĳ���ʧ��ʱ�������� errno ��
	\note ��֧�ֵ�ƽ̨����ʧ������ errno Ϊ ENOSYS ��
	\note ��������Լ�������쳣�׳��Ĳ���ʹ��ֵ��ʼ���ķ������ͱ�ʾʧ�ܽ����
	\note �� \c int Ϊ����ֵ�Ĳ������� \c -1 ��ʾʧ�ܡ�
	\note ���� NullablePointer Ҫ��
	\note ���㹲����Ҫ��
	\see WG21 N4606 17.6.3.3[nullablepointer.requirements] ��
	\see WG21 N4606 30.4.1.4[thread.sharedmutex.requirements] ��
	*/
	class WF_API FileDescriptor : private white::totally_ordered<FileDescriptor>
	{
	public:
		/*!
		\brief ɾ������
		*/
		struct WF_API Deleter
		{
			using pointer = FileDescriptor;

			void
				operator()(pointer) const wnothrow;
		};

	private:
		int desc = -1;

	public:
		wconstfn DefDeCtor(FileDescriptor)
			wconstfn
			FileDescriptor(int fd) wnothrow
			: desc(fd)
		{}
		wconstfn
			FileDescriptor(nullptr_t) wnothrow
		{}
		/*!
		\brief ���죺ʹ�ñ�׼����
		\note �Կղ��������� errno ��

		������Ϊ��ʱ�õ���ʾ��Ч�ļ��Ŀ���������������� POSIX \c fileno ������
		*/
		FileDescriptor(std::FILE*) wnothrow;

		/*!
		\note �� operator-> ��һ�£����������ã��Ա������������⡣
		*/
		PDefHOp(int, *, ) const wnothrow
			ImplRet(desc)

			PDefHOp(FileDescriptor*, ->, ) wnothrow
			ImplRet(this)
			PDefHOp(const FileDescriptor*, ->, ) const wnothrow
			ImplRet(this)

			explicit DefCvt(const wnothrow, bool, desc != -1)

			friend wconstfn WB_PURE PDefHOp(bool,
				== , const FileDescriptor& x, const FileDescriptor& y) wnothrow
			ImplRet(x.desc == y.desc)

			//! \since build 639
			friend wconstfn WB_PURE PDefHOp(bool,
				<, const FileDescriptor& x, const FileDescriptor& y) wnothrow
			ImplRet(x.desc < y.desc)

			//! \exception std::system_error ������Ч�����ʧ�ܡ�
			//@{
			/*!
			\brief ȡ����ʱ�䡣
			\return �� POSIX ʱ����ͬ��Ԫ��ʱ������
			\note ��ǰ Windows ʹ�� \c ::GetFileTime ʵ�֣�����ֻ��֤��߾�ȷ���롣
			*/
			FileTime
			GetAccessTime() const;
		/*!
		\brief ȡ�ڵ����
		\return ʧ��ʱΪ NodeCategory::Invalid ������Ϊ��Ӧ���
		*/
		NodeCategory
			GetCategory() const wnothrow;
		//@{
		/*!
		\brief ȡ��ꡣ
		\note �� POSIX ƽ̨����֧�ֲ�����
		*/
		int
			GetFlags() const;
		//! \brief ȡģʽ��
		mode_t
			GetMode() const;
		//@}
		/*!
		\return �� POSIX ʱ����ͬ��Ԫ��ʱ������
		\note ��ǰ Windows ʹ�� \c ::GetFileTime ʵ�֣�����ֻ��֤��߾�ȷ���롣
		*/
		//@{
		//! \brief ȡ�޸�ʱ�䡣
		FileTime
			GetModificationTime() const;
		//! \brief ȡ�޸ĺͷ���ʱ�䡣
		array<FileTime, 2>
			GetModificationAndAccessTime() const;
		//@}
		//@}
		/*!
		\note ���洢����ʧ�ܣ����� errno Ϊ \c ENOMEM ��
		*/
		//@{
		//! \brief ȡ�ļ�ϵͳ�ڵ��ʶ��
		FileNodeID
			GetNodeID() const wnothrow;
		//! \brief ȡ��������
		size_t
			GetNumberOfLinks() const wnothrow;
		//@}
		/*!
		\brief ȡ��С��
		\return ���ֽڼ�����ļ���С��
		\throw std::system_error ��������Ч���ļ���С��ѯʧ�ܡ�
		\throw std::invalid_argument �ļ���С��ѯ���С�� 0 ��
		\note �ǳ����ļ����ļ�ϵͳʵ�ֿ��ܳ���
		*/
		std::uint64_t
			GetSize() const;

		/*!
		\brief ���÷���ʱ�䡣
		\throw std::system_error ����ʧ�ܡ�
		\note DS ƽ̨����֧�ֲ�����
		*/
		void
			SetAccessTime(FileTime) const;
		/*!
		\throw std::system_error ����ʧ�ܻ�֧�ֲ�����
		\note �� POSIX ƽ̨����֧�ֲ�����
		*/
		//@{
		/*!
		\brief ��������ģʽ��
		\return �Ƿ���Ҫ���ı����á�
		\see http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html ��
		*/
		bool
			SetBlocking() const;
		//! \brief ������ꡣ
		void
			SetFlags(int) const;
		//! \brief ���÷���ģʽ��
		void
			SetMode(mode_t) const;
		//@}
		/*!
		\throw std::system_error ����ʧ�ܡ�
		\note DS ƽ̨����֧�ֲ�����
		\note Win32 ƽ̨��Ҫ��򿪵��ļ�����дȨ�ޡ�
		*/
		//@{
		//! \brief �����޸�ʱ�䡣
		void
			SetModificationTime(FileTime) const;
		//! \brief �����޸ĺͷ���ʱ�䡣
		void
			SetModificationAndAccessTime(array<FileTime, 2>) const;
		//@}
		/*!
		\brief ���÷�����ģʽ��
		\return �Ƿ���Ҫ���ı����á�
		\throw std::system_error ����ʧ�ܻ�֧�ֲ�����
		\note �� POSIX ƽ̨����֧�ֲ�����
		\note �Բ�֧�ַ��������ļ��������� POSIX δָ���Ƿ���� \c O_NONBLOCK ��
		\see http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html ��
		*/
		bool
			SetNonblocking() const;
		//@{
		/*!
		\brief �����ļ���ָ�����ȡ�
		\pre ָ���ļ����Ѿ��򿪲���д��
		\note ���ı��ļ���дλ�á�

		���ļ�����ָ�����ȣ���չ��ʹ�ÿ��ֽ���䣻��������ʼָ�����ȵ��ֽڡ�
		*/
		bool
			SetSize(size_t) wnothrow;
		/*!
		\brief ���÷���ģʽ��
		\note ���� Win32 �ı�ģʽ��Ϊ��ƽ̨�������ͷ���ֵ���������ͬ \c setmode ������
		\note ����ƽ̨�������á�
		*/
		int
			SetTranslationMode(int) const wnothrow;

		/*!
		\brief ˢ�¡�
		\throw std::system_error ����ʧ�ܡ�
		*/
		void
			Flush();

		/*!
		\pre ��Ӷ��ԣ��ļ���Ч��
		\pre ��Ӷ��ԣ�ָ������ǿա�
		\note ÿ�ζ�д������� errno ����дʱ�� EINTR ʱ������
		*/
		//@{
		/*!
		\brief ѭ����д�ļ���
		*/
		//@{
		/*!
		\note ÿ�ζ� 0 �ֽ�ʱ���� errno Ϊ 0 ��
		\sa Read
		*/
		WB_NONNULL(2) size_t
			FullRead(void*, size_t) wnothrowv;

		/*!
		\note ÿ��д 0 �ֽ�ʱ���� errno Ϊ ENOSPC ��
		\sa Write
		*/
		WB_NONNULL(2) size_t
			FullWrite(const void*, size_t) wnothrowv;
		//@}

		/*!
		\brief ��д�ļ���
		\note ������� errno ��
		\note Win64 ƽ̨����С�ض�Ϊ 32 λ��
		\return ����������Ϊ size_t(-1) ������Ϊ��ȡ���ֽ�����
		*/
		//@{
		WB_NONNULL(2) size_t
			Read(void*, size_t) wnothrowv;

		WB_NONNULL(2) size_t
			Write(const void*, size_t) wnothrowv;
		//@}
		//@}
		//@}

		/*!
		\brief �ڶ���������д���һ����ָ�����ļ���
		\pre ���ԣ��ļ���Ч��
		\pre ������ָ���Ļ�������С������ 0 ��
		\throw std::system_error �ļ���дʧ�ܡ�
		*/
		//@{
		//! \pre ����ָ���Ļ������ǿ��Ҵ�С������ 0 ��
		static WB_NONNULL(3) void
			WriteContent(FileDescriptor, FileDescriptor, byte*, size_t);
		/*!
		\note ���һ������ָ����������С�����ޣ�������ʧ���Զ����·��䡣
		\throw std::bad_alloc ����������ʧ�ܡ�
		*/
		static void
			WriteContent(FileDescriptor, FileDescriptor,
				size_t = wimpl(size_t(BUFSIZ << 4)));
		//@}

		/*!
		\pre ��Ӷ��ԣ��ļ���Ч��
		\note DS ƽ̨�������á�
		\note POSIX ƽ̨����ʹ�� POSIX �ļ�����ʹ�� BSD ���棬
		�Ա����޷����Ƶ��ͷŵ��°�ȫ©����
		*/
		//@{
		/*!
		\note Win32 ƽ̨�����ڴ�ӳ���ļ�ΪЭͬ���������ļ�Ϊǿ������
		\note ����ƽ̨��Эͬ����
		\warning �����ֲ�ʽ�ļ�ϵͳ���ܲ�����ȷ֧�ֶ�ռ������ Andrew File System ����
		*/
		//@{
		//! \throw std::system_error �ļ�����ʧ�ܡ�
		//@{
		void
			lock();

		void
			lock_shared();
		//@}

		//! \return �Ƿ������ɹ���
		//@{
		bool
			try_lock() wnothrowv;

		bool
			try_lock_shared() wnothrowv;
		//@}
		//@}

		//! \pre ���̶��ļ����ʾ�������Ȩ��
		//@{
		void
			unlock() wnothrowv;

		void
			unlock_shared() wnothrowv;
		//@}
		//@}
	};


	using UniqueFile = unique_ptr_from<FileDescriptor::Deleter>;


	/*!
	\brief ȡĬ��Ȩ�ޡ�
	*/
	WF_API WB_STATELESS mode_t
		DefaultPMode() wnothrow;

	/*!
	\brief ���ñ�׼��������������/���ģʽ��
	\pre ��Ӷ��ԣ������ǿա�
	*/
	//@{
	WF_API void
		SetBinaryIO(std::FILE*) wnothrowv;

	/*!
	\warning �ı�Ĭ����־Ĭ�Ϸ�����ǰ����Ӧʹ�� std::cerr �� std::clog
	������ \c stderr �������Ա��⵼��ͬ�����⡣
	\sa FetchCommonLogger
	\sa Logger::DefaultSendLog
	*/
	inline PDefH(void, SetupBinaryStdIO, std::FILE* in = stdin,
		std::FILE* out = stdout, bool sync = {}) wnothrowv
		ImplExpr(SetBinaryIO(in), SetBinaryIO(out),
			std::ios_base::sync_with_stdio(sync))
		//@}


		/*!
		\brief ISO C++ ��׼��������ӿڴ�ģʽת��Ϊ POSIX �ļ���ģʽ��
		\return ��ʧ��Ϊ 0 ������Ϊ��Ӧ��ֵ��
		*/
		//@{
		//! \note ���Զ�����ģʽ��
		WF_API WB_STATELESS int
		omode_conv(std::ios_base::openmode) wnothrow;

	/*!
	\note ��չ�������Զ�����ģʽ��
	\note POSIX ƽ̨��ͬ omode_conv ��
	*/
	WF_API WB_STATELESS int
		omode_convb(std::ios_base::openmode) wnothrow;
	//@}

	/*!
	\pre ���ԣ���һ�����ǿա�
	\note ���洢����ʧ�ܣ����� errno Ϊ \c ENOMEM ��
	\since build 669
	*/
	//@{
	/*!
	\brief ����·���ɷ����ԡ�
	\param path ·��������ͬ POSIX <tt>::open</tt> ��
	\param amode ģʽ����������ͬ POSIX.1 2004 ��������Ϊȡ����ʵ�֡� ��
	\note errno �ڳ���ʱ�ᱻ���ã�����ֵ��ʵ�ֶ��塣
	*/
	//@{
	WF_API WB_NONNULL(1) int
		uaccess(const char* path, int amode) wnothrowv;
	WF_API WB_NONNULL(1) int
		uaccess(const char16_t* path, int amode) wnothrowv;
	//@}

	/*!
	\param filename �ļ���������ͬ POSIX \c ::open ��
	\param oflag ����꣬��������ͬ POSIX.1 2004 ��������Ϊȡ����ʵ�֡�
	\param pmode ��ģʽ����������ͬ POSIX.1 2004 ��������Ϊȡ����ʵ�֡�
	*/
	//@{
	//! \brief �� UTF-8 �ļ����޻�����ļ���
	WF_API WB_NONNULL(1) int
		uopen(const char* filename, int oflag, mode_t pmode = DefaultPMode())
		wnothrowv;
	//! \brief �� UCS-2 �ļ����޻�����ļ���
	WF_API WB_NONNULL(1) int
		uopen(const char16_t* filename, int oflag, mode_t pmode = DefaultPMode())
		wnothrowv;
	//@}

	//! \param filename �ļ���������ͬ std::fopen ��
	//@{
	/*!
	\param mode ��ģʽ����������ͬ ISO C11 ��������Ϊȡ����ʵ�֡�
	\pre ���ԣ�<tt>mode && *mode != 0</tt> ��
	*/
	//@{
	//! \brief �� UTF-8 �ļ������ļ���
	WF_API WB_NONNULL(1, 2) std::FILE*
		ufopen(const char* filename, const char* mode) wnothrowv;
	//! \brief �� UCS-2 �ļ������ļ���
	WF_API WB_NONNULL(1, 2) std::FILE*
		ufopen(const char16_t* filename, const char16_t* mode) wnothrowv;
	//@}
	//! \param mode ��ģʽ������������ ISO C++11 ��Ӧ��������Ϊȡ����ʵ�֡�
	//@{
	//! \brief �� UTF-8 �ļ������ļ���
	WF_API WB_NONNULL(1) std::FILE*
		ufopen(const char* filename, std::ios_base::openmode mode) wnothrowv;
	//! \brief �� UCS-2 �ļ������ļ���
	WF_API WB_NONNULL(1) std::FILE*
		ufopen(const char16_t* filename, std::ios_base::openmode mode) wnothrowv;
	//@}

	//! \note ʹ�� ufopen ������ֻ��ģʽ�򿪲���ʵ�֡�
	//@{
	//! \brief �ж�ָ�� UTF-8 �ļ������ļ��Ƿ���ڡ�
	WF_API WB_NONNULL(1) bool
		ufexists(const char*) wnothrowv;
	//! \brief �ж�ָ�� UCS-2 �ļ������ļ��Ƿ���ڡ�
	WF_API WB_NONNULL(1) bool
		ufexists(const char16_t*) wnothrowv;
	//@}
	//@}

	/*!
	\brief ȡ��ǰ����Ŀ¼������ָ���������С�
	\param size ����������
	\return ���ɹ�Ϊ��һ����������Ϊ��ָ�롣
	\note ��������ͬ POSIX.1 2004 �� \c ::getcwd ��
	\note Win32 ƽ̨�����ҽ������Ϊ��Ŀ¼ʱ�Էָ���������
	\note ����ƽ̨��δָ������Ƿ��Էָ���������
	*/
	//@{
	//! \param buf UTF-8 ��������ʼָ�롣
	WF_API WB_NONNULL(1) char*
		ugetcwd(char* buf, size_t size) wnothrowv;
	//! \param buf UCS-2 ��������ʼָ�롣
	WF_API WB_NONNULL(1) char16_t*
		ugetcwd(char16_t* buf, size_t size) wnothrowv;
	//@}

	/*!
	\pre ���ԣ������ǿա�
	\return �����Ƿ�ɹ���
	\note errno �ڳ���ʱ�ᱻ���ã�����ֵ��������ȷ���⣬��ʵ�ֶ��塣
	\note ������ʾ·����ʹ�� UTF-8 ���롣
	\note DS ʹ�� newlib ʵ�֡� MinGW32 ʹ�� MSVCRT ʵ�֡� Android ʹ�� bionic ʵ�֡�
	���� Linux ʹ�� GLibC ʵ�֡�
	*/
	//@{
	/*!
	\brief �л���ǰ����·����ָ��·����
	\note POSIX ƽ̨����·���ͷ���ֵ������ͬ \c ::chdir ��
	*/
	WF_API WB_NONNULL(1) bool
		uchdir(const char*) wnothrowv;

	/*!
	\brief ��·����Ĭ��Ȩ���½�һ��Ŀ¼��
	\note Ȩ����ʵ�ֶ��壺 DS ʹ�����Ȩ�ޣ� MinGW32 ʹ�� \c ::_wmkdir ָ����Ĭ��Ȩ�ޡ�
	*/
	WF_API WB_NONNULL(1) bool
		umkdir(const char*) wnothrowv;

	/*!
	\brief ��·��ɾ��һ����Ŀ¼��
	\note POSIX ƽ̨����·���ͷ���ֵ������ͬ \c ::rmdir ��
	*/
	WF_API WB_NONNULL(1) bool
		urmdir(const char*) wnothrowv;

	/*!
	\brief ��·��ɾ��һ����Ŀ¼�ļ���
	\note POSIX ƽ̨����·���ͷ���ֵ������ͬ \c ::unlink ��
	\note Win32 ƽ̨��֧���Ƴ�ֻ���ļ�����ɾ���򿪵��ļ�����ʧ�ܡ�
	*/
	WF_API WB_NONNULL(1) bool
		uunlink(const char*) wnothrowv;

	/*!
	\brief ��·���Ƴ�һ���ļ���
	\note POSIX ƽ̨����·���ͷ���ֵ������ͬ \c ::remove ���Ƴ��ǿ�Ŀ¼����ʧ�ܡ�
	\note Win32 ƽ̨��֧���Ƴ���Ŀ¼��ֻ���ļ�����ɾ���򿪵��ļ�����ʧ�ܡ�
	\see https://msdn.microsoft.com/en-us/library/kc07117k.aspx ��
	*/
	WF_API WB_NONNULL(1) bool
		uremove(const char*) wnothrowv;
	//@}
	//@}
	//@}


	/*!
	\ingroup workarounds
	*/
	//@{
#if __GLIBCXX__ || WB_IMPL_MSCPP
	/*!
	\note ��չ��ģʽ��
	*/
	//@{
#	if __GLIBCXX__
	//! \brief ��ʾ�����Ѵ����ļ����������ļ���ģʽ��
	wconstexpr const auto ios_nocreate(
		std::ios_base::openmode(std::_Ios_Openmode::wimpl(_S_trunc << 1)));
	/*!
	\brief ��ʾ�������������ļ���ģʽ��
	\note �ɱ� ios_nocreate ���Ƕ�����Ч��
	*/
	wconstexpr const auto ios_noreplace(
		std::ios_base::openmode(std::_Ios_Openmode::wimpl(_S_trunc << 2)));
#	else
	wconstexpr const auto ios_nocreate(std::ios::_Nocreate);
	wconstexpr const auto ios_noreplace(std::ios::_Noreplace);
#	endif
	//@}


	template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_filebuf
#	if __GLIBCXX__
		: public wimpl(__gnu_cxx::stdio_filebuf<_tChar, _tTraits>)
#	else
		: public wimpl(std::basic_filebuf<_tChar, _tTraits>)
#	endif
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits = _tTraits;

		DefDeCtor(basic_filebuf)
#	if __GLIBCXX__
			using wimpl(__gnu_cxx::stdio_filebuf<_tChar, _tTraits>::stdio_filebuf);
#	else
			using wimpl(std::basic_filebuf<_tChar, _tTraits>::basic_filebuf);
#	endif
		DefDeCopyMoveCtorAssignment(basic_filebuf)

	public:
		/*!
		\note ������չģʽ��
		*/
		basic_filebuf<_tChar, _tTraits>*
			open(UniqueFile p, std::ios_base::openmode mode)
		{
			if (p)
			{
				mode &= ~(ios_nocreate | ios_noreplace);
#	if __GLIBCXX__
				this->_M_file.sys_open(*p.get(), mode);
				if (open_check(mode))
				{
					p.release();
					return this;
				}
#	else
				if (!this->is_open())
					if (const auto mode_str = white::openmode_conv(mode))
						if (open_file_ptr(::_fdopen(*p.get(), mode_str))) {
							p.release();
							return this;
						}
#	endif
			}
			return {};
		}
		template<typename _tPathChar>
		std::basic_filebuf<_tChar, _tTraits>*
			open(const _tPathChar* s, std::ios_base::openmode mode)
		{
			if (!this->is_open())
			{
#	if __GLIBCXX__
				this->_M_file.sys_open(uopen(s, omode_convb(mode)), mode);
				if (open_check(mode))
					return this;
#	else
				return open_file_ptr(std::_Fiopen(s, mode,
					int(std::ios_base::_Openprot)));
#	endif
			}
			return {};
		}
		template<class _tString,
			wimpl(typename = white::enable_for_string_class_t<_tString>)>
			std::basic_filebuf<_tChar, _tTraits>*
			open(const _tString& s, std::ios_base::openmode mode)
		{
			return open(s.c_str(), mode);
		}

	private:
#	if __GLIBCXX__
		bool
			open_check(std::ios_base::openmode mode)
		{
			if (this->is_open())
			{
				this->_M_allocate_internal_buffer();
				this->_M_mode = mode;
				wunseq(this->_M_reading = {}, this->_M_writing = {});
				this->_M_set_buffer(-1);
				wunseq(this->_M_state_cur = this->_M_state_beg,
					this->_M_state_last = this->_M_state_beg);
				if ((mode & std::ios_base::ate) && this->seekoff(0,
					std::ios_base::end) == pos_type(off_type(-1)))
					this->close();
				else
					return true;
				return true;
			}
			return {};
		}
#	else
		WB_NONNULL(1) basic_filebuf<_tChar, _tTraits>*
			open_file_ptr(::FILE* p_file)
		{
			if (p_file)
			{
				this->_Init(p_file, std::basic_filebuf<_tChar, _tTraits>::_Openfl);
				this->_Initcvt(
#if WB_IMPL_MSCPP < 1914 
					&
#endif
					std::use_facet<std::codecvt<_tChar, char,
					typename _tTraits::state_type>>(
						std::basic_streambuf<_tChar, _tTraits>::getloc()));
				return this;
			}
			return {};
		}
#	endif
	};


	//extern template class WF_API basic_filebuf<char>;
	//extern template class WF_API basic_filebuf<wchar_t>;

	using filebuf = basic_filebuf<char>;
	using wfilebuf = basic_filebuf<wchar_t>;


	//@{
	template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_ifstream : public std::basic_istream<_tChar, _tTraits>
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits_type = _tTraits;

	private:
		using base_type = std::basic_istream<char_type, traits_type>;

		mutable basic_filebuf<_tChar, _tTraits> fbuf{};

	public:
		basic_ifstream()
			: base_type(nullptr)
		{
			this->init(&fbuf);
		}
		template<typename _tParam,
			wimpl(typename = white::exclude_self_t<basic_ifstream, _tParam>)>
			explicit
			basic_ifstream(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in)
			: base_type(nullptr)
		{
			this->init(&fbuf);
			this->open(wforward(s), mode);
		}
		DefDelCopyCtor(basic_ifstream)
			basic_ifstream(basic_ifstream&& rhs)
			: base_type(std::move(rhs)),
			fbuf(std::move(rhs.fbuf))
		{
			base_type::set_rdbuf(&fbuf);
		}
		DefDeDtor(basic_ifstream)

			DefDelCopyAssignment(basic_ifstream)
			basic_ifstream&
			operator=(basic_ifstream&& rhs)
		{
			base_type::operator=(std::move(rhs));
			fbuf = std::move(rhs.fbuf);
			return *this;
		}

		void
			swap(basic_ifstream& rhs)
		{
			base_type::swap(rhs),
				fbuf.swap(rhs.fbuf);
		}

		void
			close()
		{
			if (!fbuf.close())
				this->setstate(std::ios_base::failbit);
		}

		bool
			is_open() const
		{
			return fbuf.is_open();
		}

		template<typename _tParam>
		void
			open(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in)
		{
			if (fbuf.open(wforward(s), mode))
				this->clear();
			else
				this->setstate(std::ios_base::failbit);
		}

		WB_ATTR_returns_nonnull std::basic_filebuf<_tChar, _tTraits>*
			rdbuf() const
		{
			return &fbuf;
		}
	};

	template<typename _tChar, class _tTraits>
	inline DefSwap(, basic_ifstream<_tChar WPP_Comma _tTraits>)


		template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_ofstream : public std::basic_ostream<_tChar, _tTraits>
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits_type = _tTraits;

	private:
		using base_type = std::basic_ostream<char_type, traits_type>;

		mutable basic_filebuf<_tChar, _tTraits> fbuf{};

	public:
		basic_ofstream()
			: base_type(nullptr)
		{
			this->init(&fbuf);
		}
		template<typename _tParam,
			wimpl(typename = white::exclude_self_t<basic_ofstream, _tParam>)>
			explicit
			basic_ofstream(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::out)
			: base_type(nullptr)
		{
			this->init(&fbuf);
			this->open(wforward(s), mode);
		}
		DefDelCopyCtor(basic_ofstream)
			basic_ofstream(basic_ofstream&& rhs)
			: base_type(std::move(rhs)),
			fbuf(std::move(rhs.fbuf))
		{
			base_type::set_rdbuf(&fbuf);
		}
		DefDeDtor(basic_ofstream)

			DefDelCopyAssignment(basic_ofstream)
			basic_ofstream&
			operator=(basic_ofstream&& rhs)
		{
			base_type::operator=(std::move(rhs));
			fbuf = std::move(rhs.fbuf);
			return *this;
		}

		void
			swap(basic_ofstream& rhs)
		{
			base_type::swap(rhs),
				fbuf.swap(rhs.fbuf);
		}

		void
			close()
		{
			if (!fbuf.close())
				this->setstate(std::ios_base::failbit);
		}

		bool
			is_open() const
		{
			return fbuf.is_open();
		}

		template<typename _tParam>
		void
			open(_tParam&& s, std::ios_base::openmode mode = std::ios_base::out)
		{
			if (fbuf.open(wforward(s), mode))
				this->clear();
			else
				this->setstate(std::ios_base::failbit);
		}

		WB_ATTR_returns_nonnull std::basic_filebuf<_tChar, _tTraits>*
			rdbuf() const
		{
			return &fbuf;
		}
	};

	template<typename _tChar, class _tTraits>
	inline DefSwap(, basic_ofstream<_tChar WPP_Comma _tTraits>)
		//@}


		template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_fstream : public std::basic_iostream<_tChar, _tTraits>
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits_type = _tTraits;

	private:
		using base_type = std::basic_iostream<char_type, traits_type>;

		mutable basic_filebuf<_tChar, _tTraits> fbuf{};

	public:
		basic_fstream()
			: base_type(nullptr)
		{
			this->init(&fbuf);
		}
		template<typename _tParam,
			wimpl(typename = white::exclude_self_t<basic_fstream, _tParam>)>
			explicit
			basic_fstream(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
			: base_type(nullptr)
		{
			this->init(&fbuf);
			this->open(wforward(s), mode);
		}
		DefDelCopyCtor(basic_fstream)
			basic_fstream(basic_fstream&& rhs)
			: base_type(std::move(rhs)),
			fbuf(std::move(rhs.fbuf))
		{
			base_type::set_rdbuf(&fbuf);
		}
		DefDeDtor(basic_fstream)

			DefDelCopyAssignment(basic_fstream)
			basic_fstream&
			operator=(basic_fstream&& rhs)
		{
			base_type::operator=(std::move(rhs));
			fbuf = std::move(rhs.fbuf);
			return *this;
		}

		void
			swap(basic_fstream& rhs)
		{
			base_type::swap(rhs),
				fbuf.swap(rhs.fbuf);
		}

		void
			close()
		{
			if (!fbuf.close())
				this->setstate(std::ios_base::failbit);
		}

		bool
			is_open() const
		{
			return fbuf.is_open();
		}

		template<typename _tParam>
		void
			open(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
		{
			if (fbuf.open(wforward(s), mode))
				this->clear();
			else
				this->setstate(std::ios_base::failbit);
		}

		WB_ATTR_returns_nonnull std::basic_filebuf<_tChar, _tTraits>*
			rdbuf() const
		{
			return &fbuf;
		}
	};

	template<typename _tChar, class _tTraits>
	inline DefSwap(, basic_fstream<_tChar WPP_Comma _tTraits>)


		//extern template class WF_API basic_fstream<char>;
		//extern template class WF_API basic_fstream<wchar_t>;

		//@{
		using ifstream = basic_ifstream<char>;
	using ofstream = basic_ofstream<char>;
	using fstream = basic_fstream<char>;
	using wifstream = basic_ifstream<wchar_t>;
	using wofstream = basic_ofstream<wchar_t>;
	//@}
	using wfstream = basic_fstream<wchar_t>;
#else
	// TODO: Use VC++ extensions to support %char16_t path initialization.
	using std::basic_filebuf;
	using std::filebuf;
	using std::wfilebuf;
	//! \since build 619
	//@{
	using std::basic_ifstream;
	using std::basic_ofstream;
	//! \since build 616
	using std::basic_fstream;
	using std::ifstream;
	using std::ofstream;
	//! \since build 616
	using std::fstream;
	using std::wifstream;
	using std::wofstream;
	//@}
	using std::wfstream;
#endif
	//@}


	//@{
	/*!
	\brief ���԰�ָ������ʼ��������Сȡ��ǰ����Ŀ¼��·����
	\pre ��Ӷ��ԣ����������� 0 ��
	\note δָ������Ƿ��Էָ���������
	*/
	template<typename _tChar>
	basic_string<_tChar>
		FetchCurrentWorkingDirectory(size_t init)
	{
		return white::retry_for_vector<basic_string<_tChar>>(init,
			[](basic_string<_tChar>& res, size_t) -> bool {
			const auto r(platform::ugetcwd(&res[0], res.length()));

			if (r)
			{
				res = r;
				return {};
			}

			const int err(errno);

			if (err != ERANGE)
				white::throw_error(err, wfsig);
			return true;
		});
	}
#if WFL_Win32
	//! \note ���������ԡ�
	//@{
	template<>
	WF_API string
		FetchCurrentWorkingDirectory(size_t);
	template<>
	WF_API u16string
		FetchCurrentWorkingDirectory(size_t);
	//@}
#endif
	//@}


	//@{
	/*!
	\note ʡ�Ե�һ����ʱΪ std::system_error ��
	*/
	//@{
	/*!
	\brief ������ֵ��ָ�������׳���һ����ָ�����͵Ķ���
	\note �ȱ����������ֵ�� errno �Ա�������еĸ�����Ӱ������
	*/
#define WCL_Raise_SysE(_t, _msg, _sig) \
	do \
	{ \
		const auto err_(errno); \
	\
		white::throw_error<_t>(err_, \
			platform::ComposeMessageWithSignature(_msg WPP_Comma _sig)); \
	}while(false)

	//! \note �����ʽ��ֵ������Ƿ�Ϊ���ʼ����ֵ��
#define WCL_RaiseZ_SysE(_t, _expr, _msg, _sig) \
	do \
	{ \
		const auto err_(_expr); \
	\
		if(err_ != decltype(err_)()) \
			white::throw_error<_t>(err_, \
				platform::ComposeMessageWithSignature(_msg WPP_Comma _sig)); \
	}while(false)
	//@}

	/*!
	\brief ���� errno ȡ�õĵ���״̬�����
	*/
#define WCL_Trace_SysE(_lv, _fn, _sig) \
	do \
	{ \
		const int err_(errno); \
	\
		TraceDe(_lv, "Failed calling " #_fn " @ %s with error %d: %s.", \
			_sig, err_, std::strerror(err_)); \
	}while(false)

	/*!
	\brief ����ϵͳ C API ���������� errno ȡ�õ���״̬�����̡�
	\pre ϵͳ C API ���ؽ���������� DefaultConstructible �� LessThanComparable Ҫ��
	\note �ȽϷ���Ĭ�Ϲ���Ľ��ֵ����ȱ�ʾ�ɹ���С�ڱ�ʾʧ�������� errno ��
	\note ����ʱֱ��ʹ��ʵ�ʲ�������ָ���Ǳ�ʶ���ı��ʽ������֤��ȫ�����ơ�
	*/
	//@{
	/*!
	\note ��ʧ���׳���һ����ָ�����͵Ķ���
	\note ʡ�Ե�һ����ʱΪ std::system_error ��
	\sa WCL_Raise_SysE
	*/
	//@{
#define WCL_WrapCall_CAPI(_t, _fn, ...) \
	[&](const char* sig_) WB_NONNULL(1){ \
		const auto res_(_fn(__VA_ARGS__)); \
	\
		if WB_UNLIKELY(res_ < decltype(res_)()) \
			WCL_Raise_SysE(_t, #_fn, sig_); \
		return res_; \
	}

#define WCL_Call_CAPI(_t, _fn, _sig, ...) \
	WCL_WrapCall_CAPI(_t, _fn, __VA_ARGS__)(_sig)

#define WCL_CallF_CAPI(_t, _fn, ...) WCL_Call_CAPI(_t, _fn, wfsig, __VA_ARGS__)
	//@}

	/*!
	\note ��ʧ�ܸ��� errno �Ľ����
	\note ��ʽת��˵����������ǰ�Ա�������Ӱ������
	\sa WCL_Trace_SysE
	*/
	//@{
#define WCL_TraceWrapCall_CAPI(_fn, ...) \
	[&](const char* sig_) WB_NONNULL(1){ \
		const auto res_(_fn(__VA_ARGS__)); \
	\
		if(WB_UNLIKELY(res_ < decltype(res_)())) \
			WCL_Trace_SysE(platform::Descriptions::Warning, _fn, sig_); \
		return res_; \
	}

#define WCL_TraceCall_CAPI(_fn, _sig, ...) \
	WCL_TraceWrapCall_CAPI(_fn, __VA_ARGS__)(_sig)

#define WCL_TraceCallF_CAPI(_fn, ...) \
	WCL_TraceCall_CAPI(_fn, wfsig, __VA_ARGS__)
	//@}
	//@}
	//@}


	//! \exception std::system_error ������Ч���ļ�ʱ���ѯʧ�ܡ�
	//@{
	/*!
	\sa FileDescriptor::GetAccessTime
	*/
	//@{
	inline WB_NONNULL(1) PDefH(FileTime, GetFileAccessTimeOf, std::FILE* fp)
		ImplRet(FileDescriptor(fp).GetAccessTime())
		/*!
		\pre ���ԣ���һ�����ǿա�
		\note ��������ʾ�������ӣ����ļ�ϵͳ֧�֣��������ӵ��ļ���������������
		*/
		//@{
		WF_API WB_NONNULL(1) FileTime
		GetFileAccessTimeOf(const char*, bool = {});
	WF_API WB_NONNULL(1) FileTime
		GetFileAccessTimeOf(const char16_t*, bool = {});
	//@}
	//@}

	/*!
	\sa FileDescriptor::GetModificationTime
	*/
	//@{
	inline WB_NONNULL(1) PDefH(FileTime, GetFileModificationTimeOf, std::FILE* fp)
		ImplRet(FileDescriptor(fp).GetModificationTime())

		/*!
		\pre ���ԣ���һ�����ǿա�
		\note ��������ʾ�������ӣ����ļ�ϵͳ֧�֣��������ӵ��ļ���������������
		*/
		//@{
		WF_API WB_NONNULL(1) FileTime
		GetFileModificationTimeOf(const char*, bool = {});
	WF_API WB_NONNULL(1) FileTime
		GetFileModificationTimeOf(const char16_t*, bool = {});
	//@}
	//@}

	/*!
	\sa FileDescriptor::GetModificationAndAccessTime
	*/
	//@{
	inline WB_NONNULL(1) PDefH(array<FileTime WPP_Comma 2>,
		GetFileModificationAndAccessTimeOf, std::FILE* fp)
		ImplRet(FileDescriptor(fp).GetModificationAndAccessTime())
		/*!
		\pre ���ԣ���һ�����ǿա�
		\note ��������ʾ�������ӣ����ļ�ϵͳ֧�֣��������ӵ��ļ���������������
		*/
		//@{
		WF_API WB_NONNULL(1) array<FileTime, 2>
		GetFileModificationAndAccessTimeOf(const char*, bool = {});
	WF_API WB_NONNULL(1) array<FileTime, 2>
		GetFileModificationAndAccessTimeOf(const char16_t*, bool = {});
	//@}
	//@}
	//@}

	/*!
	\brief ȡ·��ָ�����ļ���������
	\return ���ɹ�Ϊ���������������ļ�������ʱΪ 0 ��
	\note ��������ʾ�������ӣ����ļ�ϵͳ֧�֣��������ӵ��ļ���������������
	*/
	//@{
	WB_NONNULL(1) size_t
		FetchNumberOfLinks(const char*, bool = {});
	WB_NONNULL(1) size_t
		FetchNumberOfLinks(const char16_t*, bool = {});
	//@}


	/*!
	\brief ����ɾ��ָ��·�����ļ�������ָ��·����ģʽ���������ļ���
	\pre ��Ӷ��ԣ���һ�����ǿա�
	\note �������α�ʾĿ��·������ģʽ������Ŀ����������������Ƿ��������ǡ�
	\note �ڶ����������� Mode::User �ڡ�
	\note ��Ŀ���Ѵ��ڣ������򸲸��ļ���֤Ŀ���ļ���������������������ָ����ֵ��
	\note ��Ŀ���Ѵ��ڡ����������� 1 �Ҳ�����д�빲������ɾ��Ŀ�ꡣ
	\note ����Ŀ�겻���ڵ��µ�ɾ��ʧ�ܡ�
	\throw std::system_error ����Ŀ��ʧ�ܡ�
	*/
	WF_API WB_NONNULL(1) UniqueFile
		EnsureUniqueFile(const char*, mode_t = DefaultPMode(), size_t = 1, bool = {});
	//@}

	/*!
	\brief �Ƚ��ļ�������ȡ�
	\throw std::system_error �ļ�������ʧ�ܡ�
	\warning ��ȡʧ��ʱ���ضϷ��أ������Ҫ���бȽ��ļ���С��
	\sa IsNodeShared

	���ȱȽ��ļ��ڵ㣬��Ϊ��ͬ�ļ�ֱ����ȣ�������� errno �����ļ���ȡ���ݲ��Ƚϡ�
	*/
	//@{
	//! \note ��Ӷ��ԣ������ǿա�
	//@{
	WF_API WB_NONNULL(1, 2) bool
		HaveSameContents(const char*, const char*, mode_t = DefaultPMode());
	//! \since build 701
	WF_API WB_NONNULL(1, 2) bool
		HaveSameContents(const char16_t*, const char16_t*, mode_t = DefaultPMode());
	//@}
	/*!
	\note ʹ�ñ�ʾ�ļ����Ƶ��ַ��������������쳣��Ϣ����ʾ����Ϊ����ʡ�ԣ���
	\note ������ʾ���ļ����Կɶ���ʽ�򿪣���������ʧ�ܡ�
	\note ���ļ���ʼ�ڵ�ǰ��λ�á�
	*/
	WF_API bool
		HaveSameContents(UniqueFile, UniqueFile, const char*, const char*);
	//@}

	/*!
	\brief �жϲ����Ƿ��ʾ������ļ��ڵ㡣
	\note �������� errno ��
	*/
	//@{
	wconstfn WB_PURE PDefH(bool, IsNodeShared, const FileNodeID& x,
		const FileNodeID& y) wnothrow
		ImplRet(x != FileNodeID() && x == y)
		/*!
		\pre ��Ӷ��ԣ��ַ��������ǿա�
		\note ��������ʾ�������ӡ�
		\since build 660
		*/
		//@{
		WF_API WB_NONNULL(1, 2) bool
		IsNodeShared(const char*, const char*, bool = true) wnothrowv;
	WF_API WB_NONNULL(1, 2) bool
		IsNodeShared(const char16_t*, const char16_t*, bool = true) wnothrowv;
	//@}
	/*!
	\note ȡ�ڵ�ʧ����Ϊ������
	\sa FileDescriptor::GetNodeID
	*/
	bool
		IsNodeShared(FileDescriptor, FileDescriptor) wnothrow;
	//@}

} // namespace platform;

namespace platform_ex
{

	//@{
#if WFL_Win32
	/*!
	\brief �����ʺϱ�ʾ·���� \c char16_t �ַ�����
	\note �ַ����ͷ� \c char16_t ʱת����
	*/
	//@{
	//! \pre ��Ӷ��ԣ������ǿա�
	inline WB_NONNULL(1) PDefH(wstring, MakePathStringW, const wchar_t* s)
		ImplRet(platform::Nonnull(s))
		inline PDefH(const wstring&, MakePathStringW, const wstring& s)
		ImplRet(s)
		//! \pre ��Ӷ��ԣ������ǿա�
		WF_API WB_NONNULL(1) wstring
		MakePathStringW(const char*);
	inline PDefH(wstring, MakePathStringW, string_view sv)
		ImplRet(MakePathStringW(sv.data()))
		//@}
#else
	/*!
	\brief �����ʺϱ�ʾ·���� \c char16_t �ַ�����
	\note �ַ����ͷ� \c char16_t ʱת����
	*/
	//@{
	//! \pre ��Ӷ��ԣ������ǿա�
	inline WB_NONNULL(1) PDefH(u16string, MakePathStringU, const char16_t* s)
		ImplRet(platform::Nonnull(s))
		inline PDefH(const u16string&, MakePathStringU, const u16string& s)
		ImplRet(s)
		//! \pre ��Ӷ��ԣ������ǿա�
		WF_API WB_NONNULL(1) u16string
		MakePathStringU(const char*);
	inline PDefH(u16string, MakePathStringU, string_view sv)
		ImplRet(MakePathStringU(sv.data()))
		//@}
#endif
		//@}

} // namespace platform_ex;



#endif