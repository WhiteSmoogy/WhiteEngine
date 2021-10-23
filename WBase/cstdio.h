#ifndef WBase_cstdio_h
#define WBase_cstdio_h 1

#include "sutility.h" // for noncopyable;
#include "cassert.h" // for wdef.h, <cstdio> and wconstraint;
#include "iterator_op.hpp" // for iterator_operators_t, is_undereferenceable;

#include <cstdarg> // for std::va_list;
#include <memory> // for std::unique_ptr;
#include <ios> // for std::ios_base::openmode;

namespace white
{
	using stdex::byte;

	/*!
	\brief 关闭流缓冲。
	\pre 参数非空。
	*/
	inline int
		setnbuf(std::FILE* stream) wnothrow
	{
		wconstraint(stream);
		return std::setvbuf(stream, {}, _IONBF, 0);
	}

	/*!
	\brief 基于 ISO C 标准库的流只读迭代器。
	*/
	class WB_API ifile_iterator : public std::iterator<std::input_iterator_tag,
		byte, ptrdiff_t, const byte*, const byte&>, public white::iterator_operators_t<
		ifile_iterator, std::iterator_traits<wimpl(std::iterator<
			std::input_iterator_tag, byte, ptrdiff_t, const byte*, const byte&>) >>
	{
	protected:
		using traits_type = std::iterator<std::input_iterator_tag, byte, ptrdiff_t,
			const byte*, const byte&>;

	public:
		using char_type = byte;
		using int_type = int;

	private:
		/*!
		*/
		std::FILE* stream{};
		char_type value;

	public:
		/*!
		\brief 无参数构造。
		\post <tt>!stream</tt> 。

		构造空流迭代器。
		*/
		wconstfn
			ifile_iterator()
			: value()
		{}
		/*!
		\brief 构造：使用流指针。
		\pre 断言： <tt>ptr</tt> 。
		\post <tt>stream == ptr</tt> 。
		*/
		explicit
			ifile_iterator(std::FILE* ptr)
			: stream(ptr)
		{
			wconstraint(ptr);
			++*this;
		}
		/*!
		\brief 复制构造：默认实现。
		*/
		wconstfn
			ifile_iterator(const ifile_iterator&) = default;
		~ifile_iterator() = default;

		ifile_iterator&
			operator=(const ifile_iterator&) = default;

		wconstfn reference
			operator*() const wnothrow
		{
			return value;
		}

		/*
		\brief 前置自增。
		\pre 断言：流指针非空。
		\return 自身引用。
		\note 当读到 EOF 时置流指针为空指针。

		使用 std::fgetc 读字符。
		*/
		ifile_iterator&
			operator++();

		friend bool
			operator==(const ifile_iterator& x, const ifile_iterator& y)
		{
			return x.stream == y.stream;
		}

		wconstfn std::FILE*
			get_stream() const
		{
			return stream;
		}

		/*!
		\brief 向流中写回字符。
		*/
		//@{
		//! \pre 断言： <tt>!stream</tt> 。
		int_type
			sputbackc(char_type c)
		{
			wconstraint(stream);
			return std::ungetc(c, stream);
		}
		//! \pre 断言： <tt>!stream || steram == s</tt> 。
		int_type
			sputbackc(char_type c, std::FILE* s)
		{
			wconstraint(!stream || stream == s);
			stream = s;
			return sputbackc(c);
		}

		//! \pre 间接断言： <tt>!stream</tt> 。
		int_type
			sungetc()
		{
			return sputbackc(value);
		}
		//! \pre 间接断言： <tt>!stream || steram == s</tt> 。
		int_type
			sungetc(std::FILE* s)
		{
			return sputbackc(value, s);
		}
		//@}
	};


	/*!
	\ingroup is_undereferenceable
	\brief 判断 ifile_iterator 实例是否确定为不可解引用。
	*/
	inline bool
		is_undereferenceable(const ifile_iterator& i) wnothrow
	{
		return !i.get_stream();
	}

	/*!
	\brief ISO C/C++ 标准输入输出接口打开模式转换。
	*/
	//@{
	/*!
	\see ISO C++11 Table 132 。
	\note 忽略 std::ios_base::ate 。
	\see http://wg21.cmeerw.net/lwg/issue596 。
	*/
	WB_API WB_PURE const char*
		openmode_conv(std::ios_base::openmode) wnothrow;
	/*!
	\brief ISO C/C++ 标准输入输出接口打开模式转换。
	\return 若失败（包括空参数情形）为 std::ios_base::openmode() ，否则为对应的值。
	\see ISO C11 7.21.5.3/3 。
	\note 顺序严格限定。
	\note 支持 x 转换。
	*/
	WB_API WB_PURE std::ios_base::openmode
		openmode_conv(const char*) wnothrow;
	//@}
}


#endif
