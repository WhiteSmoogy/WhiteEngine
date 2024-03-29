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
\brief 语言实现的版本。
\since build 1.01
*/
//@{

/*!
\def WB_IMPL_CPP
\brief C++实现支持版本
\since build 1.02

定义为 __cplusplus
*/
#ifdef __cplusplus
#define WB_IMPL_CPP __cplusplus
#else
# error "This header is only for C++."
#endif



/*!
\def WB_IMPL_MSCPP
\brief Microsof C++ 实现支持版本
\since build 1.02

定义为 _MSC_VER 描述的版本号
*/

/*!
\def WB_IMPL_GNUCPP
\brief GNU C++ 实现支持版本
\since build 1.02

定义为 100进制的三重版本编号和
*/

/*!
\def WB_IMPL_CLANGCPP
\brief LLVM/Clang++ C++ 实现支持版本
\since build 1.02

定义为 100进制的三重版本编号和
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

//禁止CL编译器的安全警告
#if VC_STL
//将指针用作迭代器引发的error C4996
//See doucumentation on how to use Visual C++ 'Checked Iterators'
#undef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS 1

//使用不安全的缓存区函数引发的error C4996
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

//TODO 更改相关代码
#undef _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING 1

#undef _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING 1

#undef _ENABLE_EXTENDED_ALIGNED_STORAGE
#define _ENABLE_EXTENDED_ALIGNED_STORAGE 1
//@}

/*!
\brief 特性检测宏补充定义：若不可用则替换为预处理记号 0 。
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
\brief 预处理器通用助手宏。
\since build 1.02
*/
//@{

//\brief 替换列表。
//\note 通过括号保护，可用于传递带逗号的参数。
#define WPP_Args(...) __VA_ARGS__

//! \brief 替换为空的预处理记号。
#define WPP_Empty

/*!
\brief 替换为逗号的预处理记号。
\note 可用于代替宏的实际参数中出现的逗号。
*/
#define WPP_Comma ,

/*
\brief 记号连接。
\sa WPP_Join
*/
#define WPP_Concat(x,y) x ## y

/*
\brief 带宏替换的记号连接。
\see ISO WG21/N4140 16.3.3[cpp.concat]/3 。
\see http://gcc.gnu.org/onlinedocs/cpp/Concatenation.html 。
\see https://www.securecoding.cert.org/confluence/display/cplusplus/PRE05-CPP.+Understand+macro+replacement+when+concatenating+tokens+or+performing+stringification 。

注意 ISO C++ 未确定宏定义内 # 和 ## 操作符求值顺序。
注意宏定义内 ## 操作符修饰的形式参数为宏时，此宏不会被展开。
*/
#define WPP_Join(x,y) WPP_Concat(x,y)
//@}


/*!
\brief 实现标签。
\since build 1.02
\todo 检查语言实现的必要支持：可变参数宏。
*/
#define wimpl(...) __VA_ARGS__


/*!	\defgroup lang_impl_features Language Implementation Features
\brief 语言实现的特性。
\since build 1.02
*/
//@{

/*!
\def WB_HAS_ALIGNAS
\brief 内建 alignas 支持。
\since build 1.02
*/
#undef  WB_HAS_ALIGNAS
#define WB_HAS_ALIGNAS \
	(__has_feature(cxx_alignas) || __has_extension(cxx_alignas) || \
		WB_IMPL_CPP >= 201103L || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_ALIGNOF
\brief 内建 alignof 支持。
\since build 1.02
*/
#undef WB_HAS_ALIGNOF
#define WB_HAS_ALIGNOF (WB_IMPL_CPP >= 201103L || WB_IMPL_GNUCPP >= 40500 || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_BUILTIN_NULLPTR
\brief 内建 nullptr 支持。
\since build 1.02
*/
#undef WB_HAS_BUILTIN_NULLPTR
#define WB_HAS_BUILTIN_NULLPTR \
	(__has_feature(cxx_nullptr) || __has_extension(cxx_nullptr) || \
	 WB_IMPL_CPP >= 201103L ||  WB_IMPL_GNUCPP >= 40600 || \
	 WB_IMPL_MSCPP >= 1600)

/*!
\def WB_HAS_CONSTEXPR
\brief constexpr 支持。
\since build 1.02
*/
#undef WB_HAS_CONSTEXPR
#define WB_HAS_CONSTEXPR \
	(__has_feature(cxx_constexpr) || __has_extension(cxx_constexpr) || \
	WB_IMPL_CPP >= 201103L || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_NOEXCPT
\brief noexcept 支持。
\since build 1.02
*/
#undef WB_HAS_NOEXCEPT
#define WB_HAS_NOEXCEPT \
	(__has_feature(cxx_noexcept) || __has_extension(cxx_noexcept) || \
		WB_IMPL_CPP >= 201103L || WB_IMPL_MSCPP >= 1900)

/*!
\def WB_HAS_THREAD_LOCAL
\brief thread_local 支持。
\see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=1773 。
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
\brief 非正常终止程序。
\note 可能使用体系结构相关实现或标准库 std::abort 函数等。
\since build 1.02
*/
#if __has_builtin(__builtin_trap) || WB_IMPL_GNUCPP >= 40203
#	define WB_ABORT __builtin_trap()
#else
#	define WB_ABORT std::abort()
#endif

/*!
\def WB_ASSUME(expr)
\brief 假定表达式总是成立。
\note 若假定成立有利于优化。
\warning 若假定不成立则行为未定义。
\since build 1.02
*/
#if WB_IMPL_MSCPP >= 1200
#	define WB_ASSUME(_expr) __assume(_expr)
#elif __has_builtin(__builtin_unreachable) || WB_IMPL_GNUCPP >= 40500
#	define WB_ASSUME(_expr) (_expr) ? void(0) : __builtin_unreachable()
#else
#	define WB_ASSUME(_expr) (_expr) ? void(0) : WB_ABORT
#endif


//附加提示
//保证忽略时不导致运行时语义差异的提示,主要用于便于实现可能的优化

//属性
#if WB_IMPL_GNUCPP >= 20500
#define WB_ATTR(...) __attribute__((__VA_ARGS__))
#else
#define WB_ATTR(...)
#endif

/*!
\def WB_ATTR_STD
\brief C++ 标准属性。
\note 注意和 GNU 风格不同，在使用时受限，如不能修饰 lambda 表达式非类型的声明。
\warning 不检查指令。用户应验证可能使用的指令中的标识符在宏替换后能保持正确。
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
\brief 指示返回非空属性。
\since build 1.4
\see http://reviews.llvm.org/rL199626 。
\see http://reviews.llvm.org/rL199790 。
\todo 确认 Clang++ 最低可用的版本。
*/
#if __has_attribute(returns_nonnull) || WB_IMPL_GNUC >= 40900 \
	|| WB_IMPL_CLANGPP >= 30500
#	define WB_ATTR_returns_nonnull WB_ATTR(returns_nonnull)
#else
#	define WB_ATTR_returns_nonnull
#endif

//修饰的是分配器,或返回分配器调用的函数或函数模板
//行为类似 std::malloc 或 std::calloc
//函数若返回非空指针,返回的指针不是其他有效指针的别人,且指针指向内容不由其他存储决定
#if WB_IMPL_GNUCPP >= 20296
#	define WB_ALLOCATOR WB_ATTR(__malloc__)
#else
#	define WB_ALLOCATOR
#endif

//分支预测提示
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
\brief 指定假定保证不为空指针的函数参数。
\warning 当指定的函数实际为空时行为未定义。
*/
#if WB_IMPL_GNUCPP >= 30300
#	define WB_NONNULL(...) __attribute__ ((__nonnull__ (__VA_ARGS__)))
#else
#	define WB_NONNULL(...)
#endif

//指定无返回值函数
#if WB_IMPL_GNUCPP >= 40800
#	define WB_NORETURN [[noreturn]]
#elif WB_IMPL_GNUCPP >= 20296
#	define WB_NORETURN WB_ATTR(__noreturn__)
#else
#	define WB_NORETURN [[noreturn]]
#endif

//指定函数或函数模板实例为纯函数
//假定函数可返回,假定函数无外部可见的副作用
#if WB_IMPL_GNUCPP >= 20296
#	define WB_PURE WB_ATTR(__pure__)
#else
#	define WB_PURE
#endif

//指定函数为无状态函数
#if WB_IMPL_GNUCPP >= 20500
#	define WB_STATELESS WB_ATTR(__const__)
#else
#	define WB_STATELESS
#endif

/*!	\defgroup lib_options Library Options
\brief 库选项。
\since build 1.02
*/
//@{

/*!
\def WB_DLL
\brief 使用 WBase 动态链接库。
\since build 1.02
*/
/*!
\def WB_BUILD_DLL
\brief 构建 WBase 动态链接库。
\since build 1.02
*/
/*!
\def WB_API
\brief WBase 应用程序编程接口：用于向库文件约定链接。
\since build 1.02
\todo 判断语言特性支持。
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
\brief 使用断言。
\since build 1.02
*/
/*!
\def WB_Use_WTrace
\brief 使用调试跟踪记录。
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
\brief 使用 WBase 动态异常规范。
\since build 1.02
*/
#if 0 && !defined(NDEBUG)
#	define WB_USE_EXCEPTION_SPECIFICATION 1
#endif

/*!
\def WHITE_BEGIN
\brief 定义为 namespace white {
\since build 1.03
*/
#ifndef WHITE_BEGIN
#define WHITE_BEGIN namespace white{
#endif

/*!
\def WHITE_END
\brief 定义为 }
\since build 1.03
*/
#ifndef WHITE_END
#define WHITE_END	}
#endif

//@}

/*!	\defgrouppseudo_keyword Specified Pseudo-Keywords
\brief WBase 指定的替代关键字。
\since build 1.02
*/
//@{

/*!
\ingroup pseudo_keyword
\def walignof
\brief 查询特定类型的对齐大小。
\note 同 C++11 alignof 作用于类型时的语义。
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
\brief 指定特定类型的对齐大小。
\note 同 C++11 alignas 作用于类型时的语义。
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
\brief 指定编译时常量表达式。
\note 同 C++11 constepxr 作用于编译时常量的语义。
\since build 1.02
*/
/*!
\ingroup pseudo_keyword
\def wconstfn
\brief 指定编译时常量函数。
\note 同 C++11 constepxr 作用于编译时常量函数的语义。
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
\brief 指定编译时没有 C++11 限制和隐式成员 const 的常量函数。
\note 同 C++14 constepxr 作用于编译时常量函数的语义。
\since build 1.4
*/
#if __cpp_constexpr >= 201304
#	define wconstfn_relaxed constexpr
#else
#	define wconstfn_relaxed inline
#endif


/*!
\def wfname
\brief 展开为表示函数名的预定义变量的宏。
\todo 判断语言实现版本。
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
\brief 展开为表示函数签名的预定义变量的宏。
\todo 判断语言实现版本。
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
\brief WBase 动态异常规范：根据是否使用异常规范宏指定或忽略动态异常规范。
\note wthrow = "yielded throwing" 。
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
\brief 无异常抛出保证：指定特定的异常规范。
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
\brief 线程局部存储：若实现支持，指定为 \c thread_local 。
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
	//char无unsigned和signed指定
#if  CHAR_BIT == 8
	//一字节并不一定等于8位!
	using octet = unsigned char;
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

	//tuple,pair所需的构造重载
	using raw_tag = empty_base<>;

	//成员计算静态类型检查. 
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
	\brief 根据参数类型使用 std::forward 传递对应参数。
	\since build 1.02

	传递参数：按类型保持值类别(value catory) 和 const 修饰符。
	当表达式类型为函数或函数引用类型时，结果为左值(lvalue) ，否则：
	当且仅当左值引用类型时结果为左值（此时类型不变）；
	否则结果为对应的右值引用类型的消亡值(xvalue) 。
	*/
#define wforward(expr) std::forward<decltype(expr)>(expr)

	template<typename type, typename ...tParams>
	wconstfn auto
		unsequenced(type && arg, tParams&&...)->decltype(wforward(arg))
	{
		return wforward(arg);
	}

	//无序求值
#define wunseq stdex::unsequenced

}

#define CA_SUPPRESS( WarningNumber )

#endif