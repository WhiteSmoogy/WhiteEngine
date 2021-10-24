/*!	\file Platform.h
\ingroup Framework
\brief ƽ̨��صĹ�������޹غ�����궨�弯�ϡ�
*/

#ifndef FrameWork_Common_h
#define FrameWork_Common_h 1

#include <WFramework/WCLib/Platform.h>
#include <WBase/functional.hpp>
#include <WBase/cassert.h>
#include <WBase/cwctype.h>
#include <WBase/cstring.h>
#include <WBase/wmacro.h>

namespace platform
{

	/*!
	\ingroup Platforms
	*/
	//@{
	using ID = std::uintmax_t;

	template<ID _vN>
	using MetaID = white::integral_constant<ID, _vN>;

	//! \brief ƽ̨��ʶ�Ĺ���������ͣ�ָ������ƽ̨��
	struct IDTagBase
	{};

#if WB_IMPL_CLANGPP
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wweak-vtables"
#endif

	//! \brief ����ƽ̨��ʶ�Ĺ���������ͣ�ָ����������ƽ̨��
	struct HostedIDTagBase : virtual IDTagBase
	{};

	//! \brief ƽ̨��ʶ������������͡�
	template<ID _vN>
	struct IDTag : virtual IDTagBase, virtual MetaID<_vN>
	{};

	//! \brief ָ���ض��������͵� IDTag �ػ���
#define WFL_IDTag_Spec(_n, _base) \
	template<> \
	struct IDTag<WF_Platform_##_n> : virtual _base, \
		virtual MetaID<WF_Platform_##_n> \
	{};

	//! \brief ָ������ƽ̨�� IDTag �ػ���
#define WFL_IDTag_SpecHost(_n) \
	WFL_IDTag_Spec(_n, HostedIDTagBase)

	//! \brief ָ�������ض�ƽ̨�� IDTag �ػ���
#define WFL_IDTag_SpecOnBase(_n, _b) \
	WFL_IDTag_Spec(_n, IDTag<WF_Platform_##_b>)

	WFL_IDTag_SpecHost(Win32)
		WFL_IDTag_SpecOnBase(Win64, Win32)
		WFL_IDTag_SpecOnBase(MinGW32, Win32)
		WFL_IDTag_SpecOnBase(MinGW64, Win64)
		//WFL_IDTag_SpecHost(Linux)
		//WFL_IDTag_SpecOnBase(Linux_x86, Linux)
		//WFL_IDTag_SpecOnBase(Linux_x64, Linux)
		//WFL_IDTag_SpecOnBase(Android, Linux)
		//WFL_IDTag_SpecOnBase(Android_ARM, Android)

		//! \brief ȡƽ̨��ʶ��������Ǽ������͡�
		template<ID... _vN>
	struct IDTagSet : virtual IDTag<_vN>...
	{};
	//@}

#if WB_IMPL_CLANGPP
#	pragma GCC diagnostic pop
#endif

#define WCL_Tag_constfn inline


	/*!
	\ingroup PlatformEmulation
	\brief ��������ƽ̨ģ��Ĵ���ģ�塣
	*/
#define WCL_DefPlatformFwdTmpl(_n, _fn) \
	DefFwdTmplAuto(_n, _fn(platform::IDTag<WF_Platform>(), wforward(args)...))


	/*!
	\brief ���������ں�ʽ���õĸ����ꡣ
	\sa white::is_detected
	*/
	//@{
#define WCL_CheckDecl_t(_fn) CheckDecl##_fn##_t
#define WCL_DeclCheck_t(_fn, _call) \
	template<typename... _tParams> \
	using WCL_CheckDecl_t(_fn) \
		= decltype(_call(std::declval<_tParams&&>()...));
	//@}


	/*!
	\brief �쳣��ֹ������
	*/
	WB_NORETURN WB_API void
		terminate() wnothrow;


	  /*!
	  \brief ���Ĭ��������ָ���ַ��Ƿ�Ϊ�ɴ�ӡ�ַ���
	  \note MSVCRT �� isprint/iswprint ʵ��ȱ�ݵı�ͨ��
	  \sa https://connect.microsoft.com/VisualStudio/feedback/details/799287/isprint-incorrectly-classifies-t-as-printable-in-c-locale
	  */
	  //@{
	inline PDefH(bool, IsPrint, char c)
		ImplRet(white::isprint(c))
		inline PDefH(bool, IsPrint, wchar_t c)
		ImplRet(stdex::iswprint(c))
		template<typename _tChar>
	bool
		IsPrint(_tChar c)
	{
		return platform::IsPrint(wchar_t(c));
	}
	//@}


	//@{
	inline PDefH(white::uchar_t*, ucast, wchar_t* p) wnothrow
		ImplRet(white::replace_cast<white::uchar_t*>(p))
		inline PDefH(const white::uchar_t*, ucast, const wchar_t* p) wnothrow
		ImplRet(white::replace_cast<const white::uchar_t*>(p))
		template<typename _tChar>
	_tChar*
		ucast(_tChar* p) wnothrow
	{
		return p;
	}

	inline PDefH(wchar_t*, wcast, white::uchar_t* p) wnothrow
		ImplRet(white::replace_cast<wchar_t*>(p))
		inline PDefH(const wchar_t*, wcast, const white::uchar_t* p) wnothrow
		ImplRet(white::replace_cast<const wchar_t*>(p))
		template<typename _tChar>
	_tChar*
		wcast(_tChar* p) wnothrow
	{
		return p;
	}
	//@}

	/*!
	\brief ���ò������쳣��
	*/
	template<typename _func, typename... _tParams>
	white::invoke_result_t<_func,_tParams...>
		CallNothrow(const white::invoke_result_t<_func,_tParams...>& v, _func f,
			_tParams&&... args) wnothrowv
	{
		TryRet(f(wforward(args)...))
			CatchExpr(std::bad_alloc&, errno = ENOMEM)
			CatchIgnore(...)
			return v;
	}

	/*!
	\brief ѭ���ظ�������
	*/
	template<typename _func, typename _tErrorRef,
		typename _tError = white::decay_t<_tErrorRef>,
		typename _type = white::invoke_result_t<_func&>>
		_type
		RetryOnError(_func f, _tErrorRef&& err, _tError e = _tError())
	{
		return white::retry_on_cond([&](_type res) {
			return res < _type() && _tError(err) == e;
		}, f);
	}

	template<typename _func, typename _type = white::invoke_result_t<_func&>>
	inline _type
		RetryOnInterrupted(_func f)
	{
		return RetryOnError(f, errno, EINTR);
	}

	/*!
	\brief ִ�� UTF-8 �ַ����Ļ������
	\note ʹ�� std::system ʵ�֣�������Ϊ����� std::system ��Ϊһ�¡�
	*/
	WB_API int
		usystem(const char*);



	//@{
	/*!
	\brief ϵͳ����ѡ�
	\note �� Max ��ʼ�����Ʊ�ʾ�ɾ��е����ֵ��
	*/
	enum class SystemOption
	{
		/*!
		\brief �ڴ�ҳ���ֽ�����
		\note 0 ��ʾ��֧�ַ�ҳ��
		*/
		PageSize,
		//@{
		//! \brief һ�������пɾ��е��ź��������������
		MaxSemaphoreNumber,
		//! \brief �ɾ��е��ź��������ֵ��
		MaxSemaphoreValue,
		//@}
		//! \brief ��·�������з������ӿɱ��ɿ���������������
		MaxSymlinkLoop
	};


	/*!
	\brief ȡ�������á�
	\return ѡ�������ֵ�ܱ��������ͱ�ʾʱΪת����Ķ�Ӧѡ��ֵ������ΪĬ�ϳ���ֵ��
	\note ���� WB_STATELESS ���Σ����Ҳ�����ʱ������ʹ�ø���������档
	\note ���� WB_STATELESS ���Σ����Ҳ�����ʱ���������� errno Ϊ EINVAL ��
	\sa SystemOption
	\sa TraceDe

	������ѡ��ֵ��Ϊ������ָ����ѯ�ض�������ֵ��
	ĳЩƽ̨���ý�����Ƿ���ʱȷ���ĳ��������ô˺����޸����ã�ʹ�� WB_STATELESS ���Σ�
	����ƽ̨����������ʱȷ����ͨ����ѯ������������������ʱ�ṩ�Ľӿڣ�
	ȷ��ѡ���Ƿ���ڣ��Լ���������õ�ֵ��
	��ѡ����ڣ��Ҷ�Ӧ��ֵ���� size_t ��ʾ���򷵻�ѡ���Ӧ��ֵ��
	���򣬷���һ��Ĭ�ϵĳ���ֵ��Ϊ���ˣ���ʾ���ض��������ܿɿ�������Ĭ�����ơ�
	ע�����ֵ����ƽ̨��أ�
	���� POSIX �ṩ��Сֵ�ҷ���ѡ�������Σ�ʹ�� POSIX ��Сֵ����������ֲ�ԡ�
	����ֵ���ܲ�����ʵ��ʵ��֧�ֵ�ֵ��
	�Ա�ʾ�ɾ��е����ֵ�����ƣ�����ֵΪ \c size_t(-1) ����ʱʵ��ʵ��֧�ֵ�ֵ���ܳ���
	size_t �ı�ʾ��Χ����û��ָ����ʽ���ƣ����ܴ洢���ַ�ռ����ƣ���
	��������ֵȡ����ѡ��ľ��庬�塣
	*/
	WF_API
		size_t
		FetchLimit(SystemOption) wnothrow;
	//@}

} // namespace platform;

#endif