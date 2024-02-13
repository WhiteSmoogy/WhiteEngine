/*! \file enum.hpp
\ingroup WBase
\brief 枚举相关操作。
*/

#ifndef WBase_enum_hpp
#define WBase_enum_hpp 1

#include "tuple.hpp" // for false_type, underlying_type_t, _t, and_, or_,
//	is_enum, common_type_t, vseq::find, std::tuple;

namespace white {
	//@{
	//! \ingroup unary_type_traits
	template<typename>
	struct is_enum_union :false_type
	{};

	//! \ingroup transformation_traits
	//@{
	template<typename _type>
	struct wrapped_enum_traits {
		using type = underlying_type_t<_type>;
	};

	type_t(wrapped_enum_traits);
	//@}

	template<typename... _types>
	class enum_union
	{
		static_assert(and_<or_<is_enum<_types>, is_enum_union<_types>>...>::value,
			"Invalid types found.");

	public:
		using underlying_type = common_type_t<wrapped_enum_traits_t<_types>...>;

	private:
		underlying_type value;

	public:
		wconstfn
			enum_union() = default;
		explicit wconstfn
			enum_union(underlying_type i) wnothrow
			: value(i)
		{}
		template<typename _type,
			wimpl(typename = vseq::find<std::tuple<_types...>, _type>())>
			wconstfn
			enum_union(_type e) wnothrow
			: value(static_cast<underlying_type>(e))
		{}
		wconstfn
			enum_union(const enum_union&) = default;

		enum_union&
			operator=(const enum_union&) = default;

		explicit wconstfn
			operator underlying_type() const wnothrow
		{
			return value;
		}
	};

	//! \relates enum_union
	//@{
	template<typename... _types>
	struct is_enum_union<enum_union<_types...>> : true_type
	{};

	template<typename... _types>
	struct wrapped_enum_traits<enum_union<_types...>>
	{
		using type = typename enum_union<_types...>::underlying_type;
	};

	template<typename _type>
	wconstfn wimpl(enable_if_t)<is_enum_union<_type>::value,
		wrapped_enum_traits_t<_type >>
		underlying(_type val) wnothrow
	{
		return wrapped_enum_traits_t<_type>(val);
	}
	//@}
	//@}

	template<typename Enum>
	requires std::is_enum_v<Enum>
	constexpr underlying_type_t<Enum> underlying(Enum val) noexcept{
		return static_cast<underlying_type_t<Enum>>(val);
	}

	template<typename Enum>
	requires std::is_enum_v<Enum>
	constexpr bool has_allflags(Enum Flags, Enum Contains)
	{
		return (underlying(Flags) & underlying(Contains)) == underlying(Contains);
	}

	template<typename _type, typename Enum>
		requires std::is_enum_v<Enum>&& std::is_same_v<_type, underlying_type_t<Enum>>
	constexpr bool has_allflags(_type Flags, Enum Contains)
	{
		return (Flags & underlying(Contains)) == underlying(Contains);
	}

	template<typename Enum>
	requires std::is_enum_v<Enum>
	constexpr bool has_anyflags(Enum Flags, Enum Contains)
	{
		return (underlying(Flags) & underlying(Contains)) != 0;
	}

	template<typename _type,typename Enum>
		requires std::is_enum_v<Enum> && std::is_same_v<_type, underlying_type_t<Enum>>
	constexpr bool has_anyflags(_type Flags, Enum Contains)
	{
		return (Flags & underlying(Contains)) != 0;
	}

	template<typename Enum>
		requires std::is_enum_v<Enum>
	constexpr Enum enum_and(Enum A, Enum B)
	{
		return static_cast<Enum>(underlying(A) & underlying(B));
	}

	template<typename Enum>
		requires std::is_enum_v<Enum>
	constexpr Enum enum_or(Enum A, Enum B)
	{
		return static_cast<Enum>(underlying(A) | underlying(B));
	}
	
}
#endif