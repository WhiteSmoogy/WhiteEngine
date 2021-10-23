#pragma once

#include "wdef.h"
#include <memory>
#include <type_traits>


namespace white
{
	template<typename _type>
	wconstfn _type*
		//取内建指针
		get_raw(_type* const& p) wnothrow
	{
		return p;
	}

	template<typename _type>
	wconstfn auto
		get_raw(const std::unique_ptr<_type>& p) wnothrow -> decltype(p.get())
	{
		return p.get();
	}
	template<typename _type>
	wconstfn _type*
		get_raw(const std::shared_ptr<_type>& p) wnothrow
	{
		return p.get();
	}
	template<typename _type>
	wconstfn _type*
		get_raw(const std::weak_ptr<_type>& p) wnothrow
	{
		return p.lock().get();
	}


	/*!	\defgroup owns_any Owns Any Check
	\brief 检查是否是所有者。
	*/
	//@{
	template<typename _type>
	wconstfn bool
		owns_any(_type* const&) wnothrow
	{
		return {};
	}
	template<typename _type, typename _fDeleter>
	wconstfn bool
		owns_any(const std::unique_ptr<_type, _fDeleter>& p) wnothrow
	{
		return bool(p);
	}
	template<typename _type>
	wconstfn bool
		owns_any(const std::shared_ptr<_type>& p) wnothrow
	{
		return p.use_count() > 0;
	}
	template<typename _type>
	wconstfn bool
		owns_any(const std::weak_ptr<_type>& p) wnothrow
	{
		return !p.expired();
	}
	//@}

	/*!	\defgroup owns_nonnull Owns Nonnull Check
	\brief 检查是否是非空对象的所有者。
	*/
	//@{
	template<typename _type>
	wconstfn bool
		owns_nonnull(_type* const&) wnothrow
	{
		return {};
	}
	//! \since build 550
	template<typename _type, typename _fDeleter>
	wconstfn bool
		owns_nonnull(const std::unique_ptr<_type, _fDeleter>& p) wnothrow
	{
		return bool(p);
	}
	template<typename _type>
	wconstfn bool
		owns_nonnull(const std::shared_ptr<_type>& p) wnothrow
	{
		return bool(p);
	}
	template<typename _type>
	wconstfn bool
		owns_nonnull(const std::weak_ptr<_type>& p) wnothrow
	{
		return bool(p.lock());
	}
	//@}

	/*!	\defgroup owns_unique Owns Uniquely Check
	\brief 检查是否是唯一所有者。
	*/
	//@{
	template<typename _type>
	wconstfn bool
		owns_unique(const _type&) wnothrow
	{
		return !is_reference_wrapper<_type>();
	}
	template<typename _type, class _tDeleter>
	inline bool
		owns_unique(const std::unique_ptr<_type, _tDeleter>& p) wnothrow
	{
		return bool(p);
	}
	template<typename _type>
	inline bool
		owns_unique(const std::shared_ptr<_type>& p) wnothrow
	{
		return p.unique();
	}

	template<typename _type>
	wconstfn bool
		owns_unique_nonnull(const _type&) wnothrow
	{
		return !is_reference_wrapper<_type>();
	}
	//! \pre 参数非空。
	//@{
	template<typename _type, class _tDeleter>
	inline bool
		owns_unique_nonnull(const std::unique_ptr<_type, _tDeleter>& p) wnothrow
	{
		wconstraint(p);
		return true;
	}
	template<typename _type>
	inline bool
		owns_unique_nonnull(const std::shared_ptr<_type>& p) wnothrow
	{
		wconstraint(p);
		return p.unique();
	}
	//@}

	template<typename _type, class _tDeleter>
	inline bool
		reset(std::unique_ptr<_type, _tDeleter>& p) wnothrow
	{
		if (p.get())
		{
			p.reset();
			return true;
		}
		return false;
	}
	template<typename _type>
	inline bool
		reset(std::shared_ptr<_type>& p) wnothrow
	{
		if (p.get())
		{
			p.reset();
			return true;
		}
		return false;
	}

	template<typename _type, typename _pSrc>
	wconstfn std::unique_ptr<_type>
		//_pSrc是内建指针
		unique_raw(_pSrc* p) wnothrow
	{
		return std::unique_ptr<_type>(p);
	}
	template<typename _type>
	wconstfn std::unique_ptr<_type>
		unique_raw(_type* p) wnothrow
	{
		return std::unique_ptr<_type>(p);
	}
	template<typename _type, typename _tDeleter, typename _pSrc>
	wconstfn std::unique_ptr<_type, _tDeleter>
		unique_raw(_pSrc* p, _tDeleter&& d) wnothrow
	{
		return std::unique_ptr<_type, _tDeleter>(p, wforward(d));
	}

	template<typename _type, typename _tDeleter>
	wconstfn std::unique_ptr<_type, _tDeleter>
		unique_raw(_type* p, _tDeleter&& d) wnothrow
	{
		return std::unique_ptr<_type, _tDeleter>(p, wforward(d));
	}

	template<typename _type>
	wconstfn std::unique_ptr<_type>
		unique_raw(nullptr_t) wnothrow
	{
		return std::unique_ptr<_type>();
	}


	template<typename _type, typename... _tParams>
	wconstfn std::shared_ptr<_type>
		share_raw(_type* p, _tParams&&... args)
	{
		return std::shared_ptr<_type>(p, wforward(args)...);
	}

	template<typename _type, typename _pSrc, typename... _tParams>
	wconstfn std::shared_ptr<_type>
		share_raw(_pSrc&& p, _tParams&&... args)
	{
		static_assert(is_pointer<remove_reference_t<_pSrc>>(),
			"Invalid type found.");

		return std::shared_ptr<_type>(p, wforward(args)...);
	}
	template<typename _type>
	wconstfn std::shared_ptr<_type>
		share_raw(nullptr_t) wnothrow
	{
		return std::shared_ptr<_type>();
	}
	/*!
	\note 使用空指针和其它参数构造空对象。
	\pre 参数复制构造不抛出异常。
	*/
	template<typename _type, class... _tParams>
	wconstfn wimpl(std::enable_if_t) < sizeof...(_tParams) != 0, std::shared_ptr<_type> >
		share_raw(nullptr_t, _tParams&&... args) wnothrow
	{
		return std::shared_ptr<_type>(nullptr, wforward(args)...);
	}

	/*!
	\note 使用默认初始化。
	\see WG21 N3588 A4 。
	\since build 1.4
	*/
	//@{
	template<typename _type, typename... _tParams>
	wconstfn wimpl(std::enable_if_t<!std::is_array<_type>::value, std::unique_ptr<_type>>)
		make_unique_default_init()
	{
		return std::unique_ptr<_type>(new _type);
	}
	template<typename _type, typename... _tParams>
	wconstfn wimpl(std::enable_if_t<std::is_array<_type>::value&& std::extent<_type>::value == 0,
		std::unique_ptr<_type>>)
		make_unique_default_init(size_t size)
	{
		return std::unique_ptr<_type>(new std::remove_extent_t<_type>[size]);
	}
	template<typename _type, typename... _tParams>
	wimpl(std::enable_if_t<std::extent<_type>::value != 0>)
		make_unique_default_init(_tParams&&...) = delete;
	//@}


	//@}
}