/*!	\file Host.h
\ingroup Framework
\brief ����ƽ̨������չ��
*/

#ifndef FrameWork_Host_h
#define FrameWork_Host_h 1

#include <WFramework/WCLib/FContainer.h>
#include <WFramework/Core/WException.h>
#include <WFramework/WCLib/FReference.h>
#include <system_error>
#if !WFL_Win32
#include <WBase/FFileIO.h>
#else
using HANDLE = void *;
#endif

#if WF_Hosted

namespace platform_ex
{
	/*!
	\ingroup exceptions
	\brief �����쳣��
	*/
	class WB_API Exception : public std::system_error
	{
	private:
		platform::Descriptions::RecordLevel level = platform::Descriptions::RecordLevel::Emergent;

	public:
		/*!
		\pre ��Ӷ��ԣ��ַ���������Ӧ������ָ��ǿա�
		*/
		//@{
		WB_NONNULL(3)
			Exception(std::error_code, const char* = "unknown host exception",
				platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		Exception(std::error_code, string_view,
			platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		WB_NONNULL(4)
			Exception(int, const std::error_category&, const char*
				= "unknown host exception", platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		Exception(int, const std::error_category&, string_view,
			platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		//@}
		DefDeCopyCtor(Exception)
			/*!
			\brief ���������ඨ����Ĭ��ʵ�֡�
			*/
			~Exception() override;

		white::RecordLevel GetLevel() const wnothrow;
	};


	/*!
	\brief ���û���������
	\pre ���ԣ������ǿա�
	\warning ����֤�̰߳�ȫ��
	\throw std::system_error ����ʧ�ܡ�
	*/
	WF_API WB_NONNULL(1, 2) void
		SetEnvironmentVariable(const char*, const char*);

#	if WFL_Win32
	/*!
	\brief ���ɾ������
	*/
	struct WB_API HandleDelete
	{
		/*!
		\warning ֻ���վ����Ϊ��ֵ���Բ�ͬ�� Win32 API ������Ҫ�����顣
		\see http://blogs.msdn.com/b/oldnewthing/archive/2004/03/02/82639.aspx ��
		*/
		using pointer = ::HANDLE;

		void
			operator()(pointer) const wnothrow;
	};
//#	elif WFL_API_Has_unistd_h
//	using HandleDelete = platform::FileDescriptor::Deleter;
#	else
#		error "Unsupported platform found."
#	endif

	using UniqueHandle = platform::references::unique_ptr_from<HandleDelete>;


	//@{
	//! \brief Ĭ�����������С��
	wconstexpr const size_t DefaultCommandBufferSize(wimpl(4096));

	/*!
	\brief ȡ�����ڱ�׼����ϵ�ִ�н����
	\pre ��Ӷ��ԣ���һ�����ǿա�
	\return ��ȡ�Ķ����ƴ洢�͹رչܵ��ķ���ֵ�������Ա����õ������
	\exception std::system_error ��ȡʧ�ܡ�
	\throw std::invalid_argument �ڶ�������ֵ���� \c 0 ��
	\throw std::system_error ��ʾ��ȡʧ�ܵ��������쳣����
	\note ��һ����ָ������ڶ�����ָ��ÿ�ζ�ȡ�Ļ�������С������ִ��������м�顣
	*/
	WB_API WB_NONNULL(1) pair<string, int>
		FetchCommandOutput(const char*, size_t = DefaultCommandBufferSize);


	//! \brief ���������ִ�н���Ļ��������͡�
	using CommandCache = platform::containers::unordered_map<platform::containers::string, platform::containers::string>;

	/*!
	\brief ��������ִ�л�������
	\return ��̬����ķǿ�����ָ�롣
	*/
	WB_API white::locked_ptr<CommandCache>
		LockCommandCache();

	//@{
	//! \brief ȡ���������ִ�н����
	WB_API const string&
		FetchCachedCommandResult(const string&, size_t = DefaultCommandBufferSize);

	//! \brief ȡ���������ִ�н���ַ�����
	inline PDefH(string, FetchCachedCommandString, const string& cmd,
		size_t buf_size = DefaultCommandBufferSize)
		ImplRet(white::trail(string(FetchCachedCommandResult(cmd, buf_size))))
		//@}
		//@}

		/*!
		\brief �����ܵ���
		\throw std::system_error ��ʾ����ʧ�ܵ��������쳣����
		*/
		WB_API pair<UniqueHandle, UniqueHandle>
		MakePipe();


	/*!
	\brief ���ⲿ���������ַ������������Ϊ�ⲿ�����ַ���������
	\pre Win32 ƽ̨���ܼ�Ӷ��Բ����ǿա�

	�� Win32 ƽ̨���õ�ǰ����ҳ�� platform::MBCSToMBCS ������ַ���������ֱ�Ӵ��ݲ�����
	��ʱ�� platform::MBCSToMBCS ��ͬ��������ת��Ϊ \c string_view ʱ����ͨ�� NTCTS ���㡣
	����Ҫʹ�� <tt>const char*</tt> ָ�룬��ֱ��ʹ�� <tt>&arg[0]</tt> ����ʽ��
	*/
	//@{
#	if WFL_Win32
	WB_API WB_NONNULL(1) string
		DecodeArg(const char*);
	inline PDefH(string, DecodeArg, string_view arg)
		ImplRet(DecodeArg(&arg[0]))
#	endif
		template<typename _type
#if WFL_Win32
		, wimpl(typename = white::enable_if_t<
			!std::is_constructible<string_view, _type&&>::value>)
#endif
		>
		wconstfn auto
		DecodeArg(_type&& arg) -> decltype(wforward(arg))
	{
		return wforward(arg);
	}

#	if WFL_Win32
	WB_API WB_NONNULL(1) string
		EncodeArg(const char*);
	inline PDefH(string, EncodeArg, const string& arg)
		ImplRet(EncodeArg(&arg[0]))
#	endif
		template<typename _type
#if WFL_Win32
		, wimpl(typename = white::enable_if_t<!std::is_constructible<string,
			_type&&>::value>)
#endif
		>
		wconstfn auto
		EncodeArg(_type&& arg) -> decltype(wforward(arg))
	{
		return wforward(arg);
	}
	//@}


#	if !YCL_Android
	//@{
	/*!
	\brief �ն����ݡ�
	\note �ǹ���ʵ�֡�
	*/
	class TerminalData;

	/*!
	\brief �նˡ�
	\note �� Win32 ƽ̨ʹ�� \c tput ʵ�֣����ն˸ı䵱ǰ��Ļʱ��������δԤ�ڵ���Ϊ��
	\warning ����������
	*/
	class WB_API Terminal
	{
	public:
		/*!
		\brief �ն˽���״̬�ػ���
		*/
		class WB_API Guard final
		{
		private:
			tidy_ptr<Terminal> p_terminal;

		public:
			template<typename _fCallable, typename... _tParams>
			Guard(Terminal& te, _fCallable f, _tParams&&... args)
				: p_terminal(make_observer(&te))
			{
				if (te)
					white::invoke(f, te, wforward(args)...);
			}

			DefDeMoveCtor(Guard)

			//! \brief �����������ն����ԣ��ػ񲢼�¼����
			~Guard();

			DefDeMoveAssignment(Guard)

			DefCvt(const wnothrow, white::add_ptr_t<
					std::ios_base&(std::ios_base&)>, white::id<std::ios_base&>())
		};

	private:
		unique_ptr<TerminalData> p_data;

	public:
		/*!
		\brief ���죺ʹ�ñ�׼����Ӧ���ļ�ָ�롣
		\pre ��Ӷ��ԣ������ǿա�
		\note �� Win32 ƽ̨�¹رղ���ָ�������Ļ����Ա��� \tput ��ͬ����
		*/
		Terminal(std::FILE* = stdout);
		~Terminal();

		//! \brief �ж��ն���Ч����Ч��
		DefBoolNeg(explicit, bool(p_data))


			//@{
			//! \brief �����ʾ�����ݡ�
			bool
			Clear();

		/*!
		\brief ��ʱ�̶�ǰ��ɫ��
		\sa UpdateForeColor
		*/
		Guard
			LockForeColor(std::uint8_t);
		//@}

			bool
			RestoreAttributes();

		bool
			UpdateForeColor(std::uint8_t);
	};
	//@}

	/*!
	\brief ���ݵȼ������ն˵�ǰ��ɫ��
	\return �ն��Ƿ���Ч��
	\note ���ն���Чʱ���ԡ�
	\relates Terminal
	*/
	WB_API bool
		UpdateForeColorByLevel(Terminal&, platform::Descriptions::RecordLevel);
#	endif

} // namespace platform_ex;

#endif

#endif