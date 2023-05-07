/*! \file id.hpp
\ingroup WBase
\brief ����unique_id��

*/
#ifndef WBase_id_hpp
#define WBase_id_hpp 1


#include "wdef.h"
#include "integer_sequence.hpp"
#include <limits>
#include <tuple>
#include <functional> //for std::hash
#include <numeric> //for std::accmulate
#include <string>

#ifndef WB_IMPL_GNUCPP
#include <string_view>
#endif

namespace white {


	/*!	\defgroup hash_extensions Hash Extensions
	\brief ɢ����չ�ӿڡ�
	\note ��ǰʹ�� Boost ����Ľӿںͽ���ʵ�֡�
	\see http://www.boost.org/doc/libs/1_54_0/doc/html/hash/reference.html#boost.hash_combine ��
	*/
	//@{
	/*!
	\brief �ظ�����ɢ�С�
	\note <tt>(1UL << 31) / ((1 + std::sqrt(5)) / 4) == 0x9E3779B9</tt> ��
	\warning ʵ�֣� Boost �ĵ���Ϊ Effects �����ܸı䣬��Ӧ��Ϊ�ӿ�������
	*/
	template<typename _type>
	inline void
		hash_combine(size_t& seed, const _type& val)
		wnoexcept_spec(std::hash<_type>()(val))
	{
		seed ^= std::hash<_type>()(val) + 0x9E3779B9 + (seed << 6) + (seed >> 2);
	}

	inline void hash_combine(size_t& seed,size_t val)
	{
		seed ^= val + 0x9E3779B9 + (seed << 6) + (seed >> 2);
	}

	template<typename _type>
	wconstfn size_t
		hash_combine_seq(size_t seed, const _type& val)
		wnoexcept_spec(std::hash<_type>()(val))
	{
		return hash_combine(seed, val), seed;
	}
	template<typename _type, typename... _tParams>
	wconstfn size_t
		hash_combine_seq(size_t seed, const _type& val, const _tParams&... args)
		wnoexcept_spec(std::hash<_type>()(val))
	{
		return
			hash_combine_seq(hash_combine_seq(seed, val), args...);
	}


	template<typename _tIn>
	inline size_t
		hash(size_t seed, _tIn first, _tIn last)
	{
		return std::accumulate(first, last, seed,
			[](size_t s, decltype(*first) val) {
			hash_combine(s, val);
			return s;
		});
	}
	template<typename _tIn>
	inline size_t
		hash(_tIn first, _tIn last)
	{
		return hash(0, first, last);
	}


	template< class T, unsigned N >
	inline std::size_t hash(const T(&x)[N])
	{
		return hash(x, x + N);
	}

	template< class T, unsigned N >
	inline std::size_t hash(T(&x)[N])
	{
		return hash(x, x + N);
	}

	namespace details
	{
		//@{
		template<class, class>
		struct combined_hash_tuple;

		template<typename _type, size_t... _vSeq>
		struct combined_hash_tuple<_type, std::index_sequence<_vSeq...>>
		{
			static wconstfn size_t
				call(const _type& tp)
				wnoexcept_spec(hash_combine_seq(0, std::get<_vSeq>(tp)...))
			{
				return hash_combine_seq(0, std::get<_vSeq>(tp)...);
			}
		};
		//@}

	}

	template<typename...>
	struct combined_hash;

	template<typename _type>
	struct combined_hash<_type> : std::hash<_type>
	{};

	template<typename... _types>
	struct combined_hash<std::tuple<_types...>>
	{
		using type = std::tuple<_types...>;

		wconstfn size_t
			operator()(const type& tp) const wnoexcept_spec(
				hash_combine_seq(0, std::declval<const _types&>()...))
		{
			return details::combined_hash_tuple<type,
				index_sequence_for<_types... >> ::call(tp);
		}
	};

	template<typename _type1, typename _type2>
	struct combined_hash<std::pair<_type1, _type2>>
		: combined_hash<std::tuple<_type1, _type2>>
	{};

#ifdef WB_IMPL_MSCPP
#pragma warning(disable:4307)
#endif
	wconstexpr size_t constfn_hash(const char * str, size_t seed = 0)
	{
		return 0 == *str ? seed : constfn_hash(str + 1, seed ^ (*str + 0x9e3779b9ull + (seed << 6) + (seed >> 2)));
	}

	wconstexpr size_t constfn_hash(size_t n,const char * str, size_t seed = 0)
	{
		return 0 == n ? seed : constfn_hash(n-1,str + 1, seed ^ (*str + 0x9e3779b9ull + (seed << 6) + (seed >> 2)));
	}

	inline size_t constfn_hash(const std::string& str, size_t seed = 0)
	{
		return constfn_hash(str.c_str(),seed);
	}

#ifndef WB_IMPL_GNUCPP

	inline size_t constfn_hash(const std::string_view& str, size_t seed = 0)
	{
		return constfn_hash(str.size(),str.data(),seed);
	}
#endif


	using ucs2_t = char16_t; //!< UCS-2 �ַ����͡�
	using ucs4_t = char32_t; //!< UCS-4 �ַ����͡�
	using ucsint_t = std::char_traits<ucs4_t>::int_type; //!< UCS �������͡�
}


#endif
