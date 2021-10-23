/*! \file pointer.hpp
\ingroup WBase
\brief 通用指针。
\par 修改时间:
2017-01-02 01:37 +0800
间接扩展标准库头 <iterator> ，提供指针的迭代器适配器包装和其它模板。
*/

#ifndef WBase_pointer_hpp
#define WBase_pointer_hpp 1

#include "iterator_op.hpp" // for totally_ordered,
//	iterator_operators_t, std::iterator_traits, wconstraint;
#include "observer_ptr.hpp"
#include <functional> //for std::equal_to,std::less

namespace white
{

	//! \since build 1.4
	namespace details
	{
		template<typename _type>
		using nptr_eq1 = bool_<_type() == _type()>;
		template<typename _type>
		using nptr_eq2 = bool_<_type(nullptr) == nullptr>;

	} // namespace details;

	  //! \since build 1.4
	  //@{
	  /*!
	  \brief 可空指针包装：满足 \c NullablePointer 要求同时满足转移后为空。
	  \tparam _type 被包装的指针。
	  \pre _type 满足 \c NullablePointer 要求。
	  */
	template<typename _type>
	class nptr : private totally_ordered<nptr<_type>>
	{
		//! \since build 1.4
		static_assert(is_nothrow_copyable<_type>::value, "Invalid type found.");
		static_assert(is_destructible<_type>::value, "Invalid type found.");
		static_assert(detected_or_t<true_, details::nptr_eq1, _type>::value,
			"Invalid type found.");
#ifndef WB_IMPL_MSCPP
		static_assert(detected_or_t<true_, details::nptr_eq2, _type>::value,
			"Invalid type found.");
#endif

	public:
		using pointer = _type;

	private:
		pointer ptr{};

	public:
		nptr() = default;
		//! \since build 1.4
		//@{
		wconstfn
			nptr(std::nullptr_t) wnothrow
			: nptr()
		{}
		nptr(pointer p) wnothrow
			: ptr(p)
		{}
		//@}
		nptr(const nptr&) = default;
		nptr(nptr&& np) wnothrow
		{
			np.swap(*this);
		}

		nptr&
			operator=(const nptr&) = default;
		nptr&
			operator=(nptr&& np) wnothrow
		{
			np.swap(*this);
			return *this;
		}

		wconstfn bool
			operator!() const wnothrow
		{
			return bool(*this);
		}

		//! \since build 1.4
		//@{
		//! \pre 表达式 \c *ptr 合式。
		//@{
		wconstfn_relaxed auto
			operator*() wnothrow -> decltype(*ptr)
		{
			return *ptr;
		}
		wconstfn auto
			operator*() const wnothrow -> decltype(*ptr)
		{
			return *ptr;
		}
		//@}

		wconstfn_relaxed pointer&
			operator->() wnothrow
		{
			return ptr;
		}
		wconstfn const pointer&
			operator->() const wnothrow
		{
			return ptr;
		}
		//@}

		//! \since build 1.4
		friend wconstfn bool
			operator==(const nptr& x, const nptr& y) wnothrow
		{
			return std::equal_to<pointer>()(x.ptr, y.ptr);
		}

		//! \since build 1.4
		friend wconstfn bool
			operator<(const nptr& x, const nptr& y) wnothrow
		{
			return std::less<pointer>()(x.ptr, y.ptr);
		}

		//! \since build 1.4
		explicit wconstfn
			operator bool() const wnothrow
		{
			return bool(ptr);
		}

		//! \since build 1.4
		wconstfn const pointer&
			get() const wnothrow
		{
			return ptr;
		}

		wconstfn_relaxed pointer&
			get_ref() wnothrow
		{
			return ptr;
		}

		//! \since build 1.4
		void
			swap(nptr& np) wnothrow
		{
			using std::swap;

			swap(ptr, np.ptr);
		}
	};

	/*!
	\relates nptr
	\since build 1.4
	*/
	template<typename _type>
	inline void
		swap(nptr<_type>& x, nptr<_type>& y) wnothrow
	{
		x.swap(y);
	}
	//@}

	
	
	//@}

	template<typename _type>
	using tidy_ptr = nptr<observer_ptr<_type>>;

	/*!
	\ingroup iterator_adaptors
	\brief 指针迭代器。
	\note 转换为 bool 、有序比较等操作使用转换为对应指针实现。
	\warning 非虚析构。
	\since build 1.4

	转换指针为类类型的随机访问迭代器。
	\todo 和 std::pointer_traits 交互。
	*/
	template<typename _type>
	class pointer_iterator : public iterator_operators_t<pointer_iterator<_type>,
		std::iterator_traits<_type* >>
	{
	public:
		using iterator_type = _type*;
		using iterator_category
			= typename std::iterator_traits<iterator_type>::iterator_category;
		using value_type = typename std::iterator_traits<iterator_type>::value_type;
		using difference_type
			= typename std::iterator_traits<iterator_type>::difference_type;
		using pointer = typename std::iterator_traits<iterator_type>::pointer;
		using reference = typename std::iterator_traits<iterator_type>::reference;

	protected:
		//! \since build 1.4
		pointer raw;

	public:
		wconstfn
			pointer_iterator(nullptr_t = {})
			: raw()
		{}
		//! \since build 1.4
		template<typename _tPointer>
		wconstfn
			pointer_iterator(_tPointer&& ptr)
			: raw(wforward(ptr))
		{}
		inline
			pointer_iterator(const pointer_iterator&) = default;

		//! \since build 1.4
		//@{
		wconstfn_relaxed pointer_iterator&
			operator+=(difference_type n) wnothrowv
		{
			wconstraint(raw);
			raw += n;
			return *this;
		}

		wconstfn_relaxed pointer_iterator&
			operator-=(difference_type n) wnothrowv
		{
			wconstraint(raw);
			raw -= n;
			return *this;
		}

		//! \since build 1.4
		wconstfn reference
			operator*() const wnothrowv
		{
			return wconstraint(raw), *raw;
		}

		wconstfn_relaxed pointer_iterator&
			operator++() wnothrowv
		{
			wconstraint(raw);
			++raw;
			return *this;
		}

		wconstfn_relaxed pointer_iterator&
			operator--() wnothrowv
		{
			--raw;
			return *this;
		}

		//! \since build 1.4
		friend wconstfn bool
			operator==(const pointer_iterator& x, const pointer_iterator& y) wnothrow
		{
			return x.raw == y.raw;
		}

		//! \since build 1.4
		friend wconstfn bool
			operator<(const pointer_iterator& x, const pointer_iterator& y) wnothrow
		{
			return x.raw < y.raw;
		}

		wconstfn
			operator pointer() const wnothrow
		{
			return raw;
		}
		//@}
	};


	/*!
	\ingroup transformation_traits
	\brief 指针包装为类类型迭代器。
	\since build 1.4

	若参数是指针类型则包装为 pointer_iterator 。
	*/
	//@{
	template<typename _type>
	struct pointer_classify
	{
		using type = _type;
	};

	template<typename _type>
	struct pointer_classify<_type*>
	{
		using type = pointer_iterator<_type>;
	};
	//@}

} // namespace white;

#endif
