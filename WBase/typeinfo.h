/*! \file typeinfo.h
\ingroup WBase
\brief ISO C++ 类型信息扩展。

*/
#ifndef WBase_typeinfo_h
#define WBase_typeinfo_h 1

#include "sutility.h" //for noncopyable;
#include "type_traits.hpp" // for is_void;
#include <typeinfo> //for std::type_info,typeid;
#include <functional>

namespace white {
	//! \since build 1.4
	//@{
	/*!
	\brief 类型信息。
	\note 类似 std::type_info ，但不使用 \c name 时可避免引入冗余符号。
	\warning 非虚析构。
	*/
	class WB_API type_id_info : private wimpl(noncopyable)
	{
	public:
		using id_t = size_t;

	private:
		id_t id;

		wimpl(wconstfn)
			type_id_info(id_t i = 0)
			: id(i)
		{}

	public:
		wimpl(wconstfn) bool
			operator==(const type_id_info& rhs) const wnothrow
		{
			return id == rhs.id;
		}

		wimpl(wconstfn) bool
			operator!=(const type_id_info& rhs) const wnothrow
		{
			return id != rhs.id;
		}

		wimpl(wconstfn) bool
			before(const type_id_info& rhs) const wnothrow
		{
			return id < rhs.id;
		}

		id_t
			get() const wnothrow
		{
			return id;
		}

		size_t
			hash_code() const wnothrow
		{
			return std::hash<id_t>()(id);
		}

		template<typename _type>
		static const type_id_info&
			make()
		{
			return is_void<_type>() ? xref_void() : xref<remove_cv_t<_type>>();
		}

		wimpl(wconstfn) const char*
			name() const wnothrow
		{
			return "<unknown>";
		}

	private:
		static id_t
			push()
		{
			static id_t id;

			return ++id;
		}

		template<typename _type>
		static const type_id_info&
			xref()
		{
			static_assert(is_same<_type, remove_cv_t<_type>>(),
				"Invalid type found.");
			static type_id_info info(push());

			return info;
		}

		static const type_id_info&
			xref_void()
		{
			static type_id_info info;

			return info;
		}
	};


	/*!
	\def WB_Use_LightweightTypeID
	\brief 使用轻量类型标识代替 typeid 。
	\warning 不依赖 RTTI ；跨模块使用行为依赖实现，不保证唯一性。
	\since build 1.4
	*/
#if WB_Use_LightweightTypeID
	using type_info = type_id_info;
#else
	using std::type_info;
#endif


	//! \sa type_info
	template<typename _type>
	const type_info&
		type_id()
	{
#if WB_Use_LightweightTypeID
		return type_info::make<_type>();
#else
		return typeid(_type);
#endif
	}
	//@}
}



#endif