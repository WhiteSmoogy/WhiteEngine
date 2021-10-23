////////////////////////////////////////////////////////////////////////////
//
//  White Engine Source File.
//  Copyright (C), FNS Studios, 2014-2021.
// -------------------------------------------------------------------------
//  File name:   wdef.hpp
//  Version:     v1.03
//  Created:     10/23/2021 by WhiteSmoogy.
//  Compilers:   Visual Studio.NET 2022
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef WBase_wdef_h
#define WBase_wdef_h 1

/*!	\defgroup lang_impl_versions Language Implementation Versions
\brief ����ʵ�ֵİ汾��
\since build 1.01
*/
//@{

/*!
\def WB_IMPL_CPP
\brief C++ʵ��֧�ְ汾
\since build 1.02

����Ϊ __cplusplus
*/
#ifdef __cplusplus
#define WB_IMPL_CPP __cplusplus
#else
# error "This header is only for C++."
#endif



/*!
\def WB_IMPL_MSCPP
\brief Microsof C++ ʵ��֧�ְ汾
\since build 1.02

����Ϊ _MSC_VER �����İ汾��
*/

/*!
\def WB_IMPL_GNUCPP
\brief GNU C++ ʵ��֧�ְ汾
\since build 1.02

����Ϊ 100���Ƶ����ذ汾��ź�
*/

/*!
\def WB_IMPL_CLANGCPP
\brief LLVM/Clang++ C++ ʵ��֧�ְ汾
\since build 1.02

����Ϊ 100���Ƶ����ذ汾��ź�
*/
#ifdef _MSC_VER
#	undef WB_IMPL_MSCPP
#	define WB_IMPL_MSCPP _MSC_VER
#define VC_STL 1
#elif __clang__
#	undef WB_IMPL_CLANGPP
#	define WB_IMPL_CLANGPP (__clang_major__ * 10000 + __clang_minor__ * 100 \
			+ __clang_patchlevel__)
#	elif defined(__GNUC__)
#		undef WB_IMPL_GNUCPP
#		define WB_IMPL_GNUCPP (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 \
			+ __GNUC_PATCHLEVEL__)
#endif

#ifdef __clang__
#ifdef WB_IMPL_MSCPP
#undef WB_IMPL_MSCPP
#define VC_STL 1
#endif
#	undef WB_IMPL_CLANGPP
#	define WB_IMPL_CLANGPP (__clang_major__ * 10000 + __clang_minor__ * 100 \
			+ __clang_patchlevel__)
#endif

//��ֹCL�������İ�ȫ����
#if VC_STL
//��ָ������������������error C4996
//See doucumentation on how to use Visual C++ 'Checked Iterators'
#undef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS 1

//ʹ�ò���ȫ�Ļ���������������error C4996
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

//TODO ������ش���
#undef _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING 1

#undef _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING 1

#undef _ENABLE_EXTENDED_ALIGNED_STORAGE
#define _ENABLE_EXTENDED_ALIGNED_STORAGE 1
//@}

/*!
\brief ���Լ��겹�䶨�壺�����������滻ΪԤ����Ǻ� 0 ��
\since build 1.02
*/
//@{
//! \since build 1.4
#ifndef __has_attribute
#	define __has_attribute(...) 0
#endif

//! \since build 1.3
#ifndef __has_builtin
#	define __has_builtin(...) 0
#endif


#ifndef __has_cpp_attribute
#	define __has_cpp_attribute(...) 0
#endif

#ifndef __has_feature
#	define __has_feature(...) 0
#endif

#ifndef __has_extension
#	define __has_extension(...) 0
#endif
//@}


#include <cstddef> //std::nullptr_t,std::size_t,std::ptrdiff_t,offsetof;
#include <climits> //CHAR_BIT;
#include <cassert> //assert;
#include <cstdint>
#include <cwchar>  //std::wint_t;
#include <utility> //std::foward;
#include <type_traits> //std:is_class,std::is_standard_layout;
#include <functional> //std::function


/*!	\defgroup preprocessor_helpers Perprocessor Helpers
\brief Ԥ������ͨ�����ֺꡣ
\since build 1.02
*/
//@{

//\brief �滻�б�
//\note ͨ�����ű����������ڴ��ݴ����ŵĲ�����
#define WPP_Args(...) __VA_ARGS__

//! \brief �滻Ϊ�յ�Ԥ����Ǻš�
#define WPP_Empty

/*!
\brief �滻Ϊ���ŵ�Ԥ����Ǻš�
\note �����ڴ�����ʵ�ʲ����г��ֵĶ��š�
*/
#define WPP_Comma ,

/*
\brief �Ǻ����ӡ�
\sa WPP_Join
*/
#define WPP_Concat(x,y) x ## y

/*
\brief �����滻�ļǺ����ӡ�
\see ISO WG21/N4140 16.3.3[cpp.concat]/3 ��
\see http://gcc.gnu.org/onlinedocs/cpp/Concatenation.html ��
\see https://www.securecoding.cert.org/confluence/display/cplusplus/PRE05-CPP.+Understand+macro+replacement+when+concatenating+tokens+or+performing+stringification ��

ע�� ISO C++ δȷ���궨���� # �� ## ��������ֵ˳��
ע��궨���� ## ���������ε���ʽ����Ϊ��ʱ���˺겻�ᱻչ����
*/
#define WPP_Join(x,y) WPP_Concat(x,y)
//@}


/*!
\brief ʵ�ֱ�ǩ��
\since build 1.02
\todo �������ʵ�ֵı�Ҫ֧�֣��ɱ�����ꡣ
*/
#define wimpl(...) __VA_ARGS__


/*!	\defgroup lang_impl_features Language Implementation Features
\brief ����ʵ�ֵ����ԡ�
\since build 1.02
*/
//@{

/*!
\def WB_HAS_ALIGNAS
\brief �ڽ� alignas ֧�֡�
\since build 1.02
*/
#undef  WB_HAS_ALIGNAS
#define WB_HAS_ALIGNAS \
	(__has_feature(cxx_alignas) || __has_extension(cxx_alignas) || \
		WB_IMPL_CPP >= 201103L || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_ALIGNOF
\brief �ڽ� alignof ֧�֡�
\since build 1.02
*/
#undef WB_HAS_ALIGNOF
#define WB_HAS_ALIGNOF (WB_IMPL_CPP >= 201103L || WB_IMPL_GNUCPP >= 40500 || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_BUILTIN_NULLPTR
\brief �ڽ� nullptr ֧�֡�
\since build 1.02
*/
#undef WB_HAS_BUILTIN_NULLPTR
#define WB_HAS_BUILTIN_NULLPTR \
	(__has_feature(cxx_nullptr) || __has_extension(cxx_nullptr) || \
	 WB_IMPL_CPP >= 201103L ||  WB_IMPL_GNUCPP >= 40600 || \
	 WB_IMPL_MSCPP >= 1600)

/*!
\def WB_HAS_CONSTEXPR
\brief constexpr ֧�֡�
\since build 1.02
*/
#undef WB_HAS_CONSTEXPR
#define WB_HAS_CONSTEXPR \
	(__has_feature(cxx_constexpr) || __has_extension(cxx_constexpr) || \
	WB_IMPL_CPP >= 201103L || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_NOEXCPT
\brief noexcept ֧�֡�
\since build 1.02
*/
#undef WB_HAS_NOEXCEPT
#define WB_HAS_NOEXCEPT \
	(__has_feature(cxx_noexcept) || __has_extension(cxx_noexcept) || \
		WB_IMPL_CPP >= 201103L || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_THREAD_LOCAL
\brief thread_local ֧�֡�
\see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=1773 ��
\since build 425
*/
#undef WB_HAS_THREAD_LOCAL
#define WB_HAS_THREAD_LOCAL \
	(__has_feature(cxx_thread_local) || (WB_IMPL_CPP >= 201103L \
		&& !WB_IMPL_GNUCPP) || (WB_IMPL_GNUCPP >= 40800 && _GLIBCXX_HAVE_TLS) ||\
		WB_IMPL_MSCPP >= 1900)
//@}

/*!
\def WB_ABORT
\brief ��������ֹ����
\note ����ʹ����ϵ�ṹ���ʵ�ֻ��׼�� std::abort �����ȡ�
\since build 1.02
*/
#if __has_builtin(__builtin_trap) || WB_IMPL_GNUCPP >= 40203
#	define WB_ABORT __builtin_trap()
#else
#	define WB_ABORT std::abort()
#endif

/*!
\def WB_ASSUME(expr)
\brief �ٶ����ʽ���ǳ�����
\note ���ٶ������������Ż���
\warning ���ٶ�����������Ϊδ���塣
\since build 1.02
*/
#if WB_IMPL_MSCPP >= 1200
#	define WB_ASSUME(_expr) __assume(_expr)
#elif __has_builtin(__builtin_unreachable) || WB_IMPL_GNUCPP >= 40500
#	define WB_ASSUME(_expr) (_expr) ? void(0) : __builtin_unreachable()
#else
#	define WB_ASSUME(_expr) (_expr) ? void(0) : WB_ABORT
#endif


//������ʾ
//��֤����ʱ����������ʱ����������ʾ,��Ҫ���ڱ���ʵ�ֿ��ܵ��Ż�

//����
#if WB_IMPL_GNUCPP >= 20500
#define WB_ATTR(...) __attribute__((__VA_ARGS__))
#else
#define WB_ATTR(...)
#endif

/*!
\def WB_ATTR_STD
\brief C++ ��׼���ԡ�
\note ע��� GNU ���ͬ����ʹ��ʱ���ޣ��粻������ lambda ���ʽ�����͵�������
\warning �����ָ��û�Ӧ��֤����ʹ�õ�ָ���еı�ʶ���ں��滻���ܱ�����ȷ��
*/
#if __cpp_attributes >= 200809 || __has_feature(cxx_attributes)
#	define WB_ATTR_STD(...) [[__VA_ARGS__]]
#else
#	define WB_ATTR_STD(...)
#endif

#if __has_cpp_attribute(fallthrough)
#	define WB_ATTR_fallthrough WB_ATTR_STD(fallthrough)
#elif __has_cpp_attribute(clang::fallthrough)
#	define WB_ATTR_fallthrough WB_ATTR_STD(clang::fallthrough)
#elif __has_cpp_attribute(gnu::fallthrough)
#	define WB_ATTR_fallthrough WB_ATTR_STD(gnu::fallthrough)
#elif __has_attribute(fallthrough)
#	define WB_ATTR_fallthrough WB_ATTR(fallthrough)
#else
#	define WB_ATTR_fallthrough
#endif


/*!
\def WB_ATTR_returns_nonnull
\brief ָʾ���طǿ����ԡ�
\since build 1.4
\see http://reviews.llvm.org/rL199626 ��
\see http://reviews.llvm.org/rL199790 ��
\todo ȷ�� Clang++ ��Ϳ��õİ汾��
*/
#if __has_attribute(returns_nonnull) || WB_IMPL_GNUC >= 40900 \
	|| WB_IMPL_CLANGPP >= 30500
#	define WB_ATTR_returns_nonnull WB_ATTR(returns_nonnull)
#else
#	define WB_ATTR_returns_nonnull
#endif

//���ε��Ƿ�����,�򷵻ط��������õĺ�������ģ��
//��Ϊ���� std::malloc �� std::calloc
//���������طǿ�ָ��,���ص�ָ�벻��������Чָ��ı���,��ָ��ָ�����ݲ��������洢����
#if WB_IMPL_GNUCPP >= 20296
#	define WB_ALLOCATOR WB_ATTR(__malloc__)
#else
#	define WB_ALLOCATOR
#endif

//��֧Ԥ����ʾ
#if WB_IMPL_GNUCPP >= 29600
#	define WB_EXPECT(expr, constant) (__builtin_expect(expr, constant))
#	define WB_LIKELY(expr) (__builtin_expect(bool(expr), 1))
#	define WB_UNLIKELY(expr) (__builtin_expect(bool(expr), 0))
#else
#	define WB_EXPECT(expr, constant) (expr)
#if __has_cpp_attribute(likely)
#	define WB_LIKELY(expr) (expr) [[likely]]
#	define WB_UNLIKELY(expr) (expr) [[unlikely]]
#else
#	define WB_LIKELY(expr) (expr)
#	define WB_UNLIKELY(expr) (expr)
#endif
#endif

/*!
\def YB_NONNULL
\brief ָ���ٶ���֤��Ϊ��ָ��ĺ���������
\warning ��ָ���ĺ���ʵ��Ϊ��ʱ��Ϊδ���塣
*/
#if WB_IMPL_GNUCPP >= 30300
#	define WB_NONNULL(...) __attribute__ ((__nonnull__ (__VA_ARGS__)))
#else
#	define WB_NONNULL(...)
#endif

//ָ���޷���ֵ����
#if WB_IMPL_GNUCPP >= 40800
#	define WB_NORETURN [[noreturn]]
#elif WB_IMPL_GNUCPP >= 20296
#	define WB_NORETURN WB_ATTR(__noreturn__)
#else
#	define WB_NORETURN [[noreturn]]
#endif

//ָ����������ģ��ʵ��Ϊ������
//�ٶ������ɷ���,�ٶ��������ⲿ�ɼ��ĸ�����
#if WB_IMPL_GNUCPP >= 20296
#	define WB_PURE WB_ATTR(__pure__)
#else
#	define WB_PURE
#endif

//ָ������Ϊ��״̬����
#if WB_IMPL_GNUCPP >= 20500
#	define WB_STATELESS WB_ATTR(__const__)
#else
#	define WB_STATELESS
#endif

/*!	\defgroup lib_options Library Options
\brief ��ѡ�
\since build 1.02
*/
//@{

/*!
\def WB_DLL
\brief ʹ�� WBase ��̬���ӿ⡣
\since build 1.02
*/
/*!
\def WB_BUILD_DLL
\brief ���� WBase ��̬���ӿ⡣
\since build 1.02
*/
/*!
\def WB_API
\brief WBase Ӧ�ó����̽ӿڣ���������ļ�Լ�����ӡ�
\since build 1.02
\todo �ж���������֧�֡�
*/
#if defined(WB_DLL) && defined(WB_BUILD_DLL)
#	error "DLL could not be built and used at the same time."
#endif

#if WB_IMPL_MSCPP \
	|| (WB_IMPL_GNUCPP && (defined(__MINGW32__) || defined(__CYGWIN__)))
#	ifdef WB_DLL
#		define WB_API __declspec(dllimport)
#	elif defined(WB_BUILD_DLL)
#		define WB_API __declspec(dllexport)
#	else
#		define WB_API
#	endif
#elif defined(WB_BUILD_DLL) && (WB_IMPL_GNUCPP >= 40000 || WB_IMPL_CLANGPP)
#	define WB_API WB_ATTR(__visibility__("default"))
#else
#	define WB_API
#endif

/*!
\def WB_Use_WAssert
\brief ʹ�ö��ԡ�
\since build 1.02
*/
/*!
\def WB_Use_WTrace
\brief ʹ�õ��Ը��ټ�¼��
\since build 1.02
*/
#ifndef NDEBUG
#	ifndef WB_Use_WAssert
#		define WB_Use_WAssert 1
#	endif
#endif
#define WB_Use_WTrace 1


/*!
\def WB_USE_EXCEPTION_SPECIFICATION
\brief ʹ�� WBase ��̬�쳣�淶��
\since build 1.02
*/
#if 0 && !defined(NDEBUG)
#	define WB_USE_EXCEPTION_SPECIFICATION 1
#endif

/*!
\def WHITE_BEGIN
\brief ����Ϊ namespace white {
\since build 1.03
*/
#ifndef WHITE_BEGIN
#define WHITE_BEGIN namespace white{
#endif

/*!
\def WHITE_END
\brief ����Ϊ }
\since build 1.03
*/
#ifndef WHITE_END
#define WHITE_END	}
#endif

//@}

/*!	\defgrouppseudo_keyword Specified Pseudo-Keywords
\brief WBase ָ��������ؼ��֡�
\since build 1.02
*/
//@{

/*!
\ingroup pseudo_keyword
\def walignof
\brief ��ѯ�ض����͵Ķ����С��
\note ͬ C++11 alignof ����������ʱ�����塣
\since build 1.02
*/
#if WB_HAS_ALIGNOF
#	define walignof alignof
#else
#	define walignof(_type) std::alignment_of<_type>::value
#endif

#if WB_IMPL_MSCPP 
#define wrestrict __restrict
#elif WB_IMPL_GNUCPP
#define wrestrict __restrict__ 
#else
#define wrestrict
#endif

/*!
\ingroup pseudo_keyword
\def walignas
\brief ָ���ض����͵Ķ����С��
\note ͬ C++11 alignas ����������ʱ�����塣
\since build 1.02
*/
#if WB_HAS_ALIGNAS
#	define walignas(_n) alignas(_n)
#else
#	define walignas(_n) _declspec(align(_n))
#endif

/*!
\ingroup pseudo_keyword
\def wconstexpr
\brief ָ������ʱ�������ʽ��
\note ͬ C++11 constepxr �����ڱ���ʱ���������塣
\since build 1.02
*/
/*!
\ingroup pseudo_keyword
\def wconstfn
\brief ָ������ʱ����������
\note ͬ C++11 constepxr �����ڱ���ʱ�������������塣
\since build 1.02
*/
#if WB_HAS_CONSTEXPR
#	define wconstexpr constexpr
#	define wconstfn constexpr
#else
#	define wconstexpr const
#	define wconstfn inline
#endif

/*!
\def wconstfn_relaxed
\brief ָ������ʱû�� C++11 ���ƺ���ʽ��Ա const �ĳ���������
\note ͬ C++14 constepxr �����ڱ���ʱ�������������塣
\since build 1.4
*/
#if __cpp_constexpr >= 201304
#	define wconstfn_relaxed constexpr
#else
#	define wconstfn_relaxed inline
#endif


/*!
\def wfname
\brief չ��Ϊ��ʾ��������Ԥ��������ĺꡣ
\todo �ж�����ʵ�ְ汾��
*/
#if WB_IMPL_MSCPP || __INTEL_COMPILER >= 600 || __IBMCPP__ >= 500
#	define wfname __FUNCTION__
#elif __BORLANDC__ >= 0x550
#	define wfname __FUNC__
#elif __cplusplus >= 201103 || __STDC_VERSION__ >= 199901
#	define wfname __func__
#else
#	define wfname "<unknown-fn>"
#endif

/*!
\def wfsig
\brief չ��Ϊ��ʾ����ǩ����Ԥ��������ĺꡣ
\todo �ж�����ʵ�ְ汾��
*/
#if WB_IMPL_GNUCPP || WB_IMPL_CLANGPP
#	define wfsig __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#	define wfsig __FUNCSIG__
#elif __INTEL_COMPILER >= 600 || __IBMCPP__ >= 500
#	define wfsig __FUNCTION__
#elif __BORLANDC__ >= 0x550
#	define wfsig __FUNC__
#elif WB_IMPL_MSCPP || __cplusplus >= 201103 || __STDC_VERSION__ >= 199901
#	define wfsig __func__
#else
#	define wfsig "<unknown-fsig>"
#endif

/*!
\ingroup pseudo_keyword
\def wthrow
\brief WBase ��̬�쳣�淶�������Ƿ�ʹ���쳣�淶��ָ������Զ�̬�쳣�淶��
\note wthrow = "yielded throwing" ��
*/
#if WB_USE_EXCEPTION_SPECIFICATION
#	define wthrow throw
#else
#	define wthrow(...)
#endif

#ifdef WB_USE_EXCEPTION_VALIDATION
#	define wnothrowv wnothrow
#else
#	define wnothrowv
#endif

/*!
\ingroup pseudo_keyword
\def wnoexcept
\brief ���쳣�׳���֤��ָ���ض����쳣�淶��
\since build 1.02
*/
#if WB_HAS_NOEXCEPT
#	define wnoexcept noexcept
#	define wnoexcept_assert(_msg, ...) \
	static_assert(noexcept(__VA_ARGS__), _msg)
#else
#	define wnoexcept(...)
#	define wnoexcept_assert(_msg, ...)
#endif

#if WB_HAS_NOEXCEPT
#	define wnothrow wnoexcept
#elif WB_IMPL_GNUCPP >= 30300
#	define wnothrow __attribute__ ((nothrow))
#else
#	define wnothrow wthrow()
#endif

#define wnoexcept_spec(...) wnoexcept(noexcept(__VA_ARGS__))

/*!
\ingroup pseudo_keyword
\def wthread
\brief �ֲ߳̾��洢����ʵ��֧�֣�ָ��Ϊ \c thread_local ��
\since build 1.02
*/
#if WB_HAS_THREAD_LOCAL && defined(_MT)
#	define wthread thread_local
#else
#ifdef WB_IMPL_MSCPP
#	define wthread __declspec(thread)
#else
#	define wthread static
#endif
#endif
//@}

#ifdef WB_IMPL_MSCPP
#define wselectany __declspec(selectany)
#else
#define wselectany __attribute__((weak))
#endif

namespace stdex
{

	//char��unsigned��signedָ��
	using byte = unsigned char;
#if  CHAR_BIT == 8
	//һ�ֽڲ���һ������8λ!
	using octet = byte;
#else
	using octet = void;
#endif
	using errno_t = int;
	using std::ptrdiff_t;
	using std::size_t;
	using std::wint_t;


#if WB_HAS_BUILTIN_NULLPTR
	using std::nullptr_t;
#else
	const class nullptr_t
	{
	public:
		template<typename _type>
		inline
			operator _type*() const
		{
			return 0;
		}

		template<class _tClass, typename _type>
		inline
			operator _type _tClass::*() const
		{
			return 0;
		}
		template<typename _type>
		bool
			equals(const _type& rhs) const
		{
			return rhs == 0;
		}

		void operator&() const = delete;
	} nullptr = {};

	template<typename _type>
	inline bool
		operator==(nullptr_t lhs, const _type& rhs)
	{
		return lhs.equals(rhs);
	}
	template<typename _type>
	inline bool
		operator==(const _type& lhs, nullptr_t rhs)
	{
		return rhs.equals(lhs);
	}

	template<typename _type>
	inline bool
		operator!=(nullptr_t lhs, const _type& rhs)
	{
		return !lhs.equals(rhs);
	}
	template<typename _type>
	inline bool
		operator!=(const _type& lhs, nullptr_t rhs)
	{
		return !rhs.equals(lhs);
	}
#endif

	template<typename...>
	struct empty_base
	{};

	//tuple,pair����Ĺ�������
	using raw_tag = empty_base<>;

	//��Ա���㾲̬���ͼ��. 
	template<bool bMemObjPtr, bool bNoExcept, typename T>
	class offsetof_check
	{
		static_assert(std::is_class<T>::value, "Non class type found.");
		static_assert(std::is_standard_layout<T>::value,
			"Non standard layout type found.");
		static_assert(bMemObjPtr, "Non-static member object violation found.");
		static_assert(bNoExcept, "Exception guarantee violation found.");
	};

#define wunused(...) static_cast<void>(__VA_ARGS__)

#define woffsetof(type,member) \
	(decltype(sizeof(stdex::offsetof_check<std::is_member_object_pointer< \
	decltype(&type::member)>::value,wnoexcept(offsetof(type,member)), \
	type>))(offsetof(type,member)))

	/*!
	\ingroup pseudo_keyword
	\brief ���ݲ�������ʹ�� std::forward ���ݶ�Ӧ������
	\since build 1.02

	���ݲ����������ͱ���ֵ���(value catory) �� const ���η���
	�����ʽ����Ϊ����������������ʱ�����Ϊ��ֵ(lvalue) ������
	���ҽ�����ֵ��������ʱ���Ϊ��ֵ����ʱ���Ͳ��䣩��
	������Ϊ��Ӧ����ֵ�������͵�����ֵ(xvalue) ��
	*/
#define wforward(expr) std::forward<decltype(expr)>(expr)

	template<typename type, typename ...tParams>
	wconstfn auto
		unsequenced(type && arg, tParams&&...)->decltype(wforward(arg))
	{
		return wforward(arg);
	}

	//������ֵ
#define wunseq stdex::unsequenced

}

#define CA_SUPPRESS( WarningNumber )

#endif