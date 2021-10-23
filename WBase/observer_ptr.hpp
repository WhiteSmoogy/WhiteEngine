#pragma once

#include "cassert.h"
#include <type_traits>
#include <compare>

namespace white
{
	//! \since build 1.4
	//@{
	/*!
	\brief 观察者指针：无所有权的智能指针。
	\see WG21 N4529 8.12[memory.observer.ptr] 。
	*/
	template<typename _type>
	class observer_ptr
	{
	public:
		using element_type = _type;
		using pointer = wimpl(std::add_pointer_t<_type>);
		using reference = wimpl(std::add_lvalue_reference_t<_type>);

	private:
		_type* ptr{};

	public:
		//! \post <tt>get() == nullptr</tt> 。
		//@{
		wconstfn
			observer_ptr() wnothrow wimpl(= default);
		wconstfn
			observer_ptr(nullptr_t) wnothrow
			: ptr()
		{}
		//@}
		explicit wconstfn
			observer_ptr(pointer p) wnothrow
			: ptr(p)
		{}
		template<typename _tOther>
		wconstfn
			observer_ptr(observer_ptr<_tOther> other) wnothrow
			: ptr(other.get())
		{}

		//! \pre 断言： <tt>get() != nullptr</tt> 。
		wconstfn reference
			operator*() const wnothrowv
		{
			return wconstraint(get() != nullptr), * ptr;
		}

		wconstfn pointer
			operator->() const wnothrow
		{
			return ptr;
		}

		//! \since build 1.4
		friend wconstfn bool
			operator==(observer_ptr p, nullptr_t) wnothrow
		{
			return !p.ptr;
		}

		explicit wconstfn
			operator bool() const wnothrow
		{
			return ptr;
		}

		explicit wconstfn
			operator pointer() const wnothrow
		{
			return ptr;
		}

		wconstfn pointer
			get() const wnothrow
		{
			return ptr;
		}

		wconstfn_relaxed pointer
			release() wnothrow
		{
			const auto res(ptr);

			reset();
			return res;
		}

		wconstfn_relaxed void
			reset(pointer p = {}) wnothrow
		{
			ptr = p;
		}

		wconstfn_relaxed void
			swap(observer_ptr& other) wnothrow
		{
			std::swap(ptr, other.ptr);
		}

		friend auto operator<=>(const observer_ptr<_type>& p1, const observer_ptr<_type>& p2) = default;
		friend bool operator==(const observer_ptr<_type>& p1, const observer_ptr<_type>& p2) = default;
	};


	template<typename _type>
	auto operator<=>(const observer_ptr<_type>& p1, std::nullptr_t)
		->decltype(p1.get() <=> nullptr)
	{
		return p1.get() <=> nullptr;
	}

	template<typename _type>
	inline void
		swap(observer_ptr<_type>& p1, observer_ptr<_type>& p2) wnothrow
	{
		p1.swap(p2);
	}
	template<typename _type>
	inline observer_ptr<_type>
		make_observer(_type* p) wnothrow
	{
		return observer_ptr<_type>(p);
	}

	//@}
}

namespace std
{

	/*!
	\brief white::observer_ptr 散列支持。
	\see ISO WG21 N4529 8.12.7[memory.observer.ptr.hash] 。
	\since build 1.4
	*/
	//@{
	template<typename>
	struct hash;

	template<typename _type>
	struct hash<white::observer_ptr<_type>>
	{
		size_t
			operator()(const white::observer_ptr<_type>& p) const wimpl(wnothrow)
		{
			return hash<_type*>(p.get());
		}
	};
	//@}

} // namespace std;