#pragma once

#include "winttype.hpp"
#include "type_traits.hpp"

namespace white
{
	/*!
	\ingroup metafunctions
	\brief λ�ӱ���չ��
	\note �����ڶ������˳����м����͡�
	\todo ʹ����չ�������ͱ��� 64 λ���;��ȡ�
	*/
	//@{
	template<typename _type, bool _bSigned = is_signed<_type>::value>
	struct make_widen_int
	{
	private:
		using width = integer_width<_type>;

	public:
		using type = _t<make_signed_c<_t<make_width_int<(width::value << 1) <= 64
			? width::value << 1 : width::value>>, _bSigned>>;
	};

	/*!
	\brief �����������͡�
	\note ͬ common_type ��������ܣ������Զ���չ����λ�������С��ֵ��Χ��
	*/
	//@{
	template<typename... _types>
	struct common_int_type : common_type<_types...>
	{};

	template<typename _type1, typename _type2, typename... _types>
	struct common_int_type<_type1, _type2, _types...>
	{
	private:
		using common_t = common_type_t<_type1, _type2>;

	public:
		using type = typename common_int_type<cond_t<
			and_<is_unsigned<common_t>, or_<is_signed<_type1>, is_signed<_type2>>>,
			_t<make_widen_int<common_t, true>>, common_t>, _types...>::type;
	};
	//@}

	template<typename _type>
	struct modular_arithmetic
	{
		static wconstexpr _type value = is_unsigned<_type>::value
			? std::numeric_limits<_type>::max() : _type(0);
	};

	template<typename _type1, typename _type2>
	struct have_same_modulo : integral_constant<bool, uintmax_t(modular_arithmetic<
		_type1>::value) != 0 && uintmax_t(modular_arithmetic<_type1>::value)
		== uintmax_t(modular_arithmetic<_type2>::value)>
	{};



	template<size_t _vWidth, typename _tIn>
	typename make_width_int<_vWidth>::unsigned_type
		pack_uint(_tIn first, _tIn last) wnothrowv
	{
		static_assert(_vWidth != 0 && _vWidth % std::numeric_limits<byte>::digits
			== 0, "Invalid integer width found.");
		using utype = typename make_width_int<_vWidth>::unsigned_type;

		wconstraint(!is_undereferenceable(first));
		return std::accumulate(first, last, utype(), [](utype x, byte y) {
			return utype(x << std::numeric_limits<byte>::digits | y);
			});
	}

	template<size_t _vWidth, typename _tOut>
	void
		unpack_uint(typename make_width_int<_vWidth>::unsigned_type value,
			_tOut result) wnothrow
	{
		static_assert(_vWidth != 0 && _vWidth % std::numeric_limits<byte>::digits
			== 0, "Invalid integer width found.");
		auto n(_vWidth);

		while (!(_vWidth < (n -= std::numeric_limits<byte>::digits)))
		{
			wconstraint(!is_undereferenceable(result));
			*result = byte(value >> n);
			++result;
		}
	}

	template<size_t _vWidth>
	inline WB_NONNULL(1) typename make_width_int<_vWidth>::unsigned_type
		read_uint_le(const byte* buf) wnothrowv
	{
		wconstraint(buf);
		return white::pack_uint<_vWidth>(white::make_reverse_iterator(
			buf + _vWidth / std::numeric_limits<byte>::digits),
			white::make_reverse_iterator(buf));
	}

	/*!
	\brief ���ֽڻ�����д��ָ����Ĵ�����޷���������
	*/
	template<size_t _vWidth>
	inline WB_NONNULL(1) void
		write_uint_be(byte* buf, typename make_width_int<_vWidth>::unsigned_type
			val) wnothrowv
	{
		wconstraint(buf);
		white::unpack_uint<_vWidth>(val, buf);
	}

	//! \brief ���ֽڻ�����д��ָ�����С�����޷���������
	template<size_t _vWidth>
	inline WB_NONNULL(1) void
		write_uint_le(byte* buf, typename make_width_int<_vWidth>::unsigned_type
			val) wnothrowv
	{
		wconstraint(buf);
		white::unpack_uint<_vWidth>(val, white::make_reverse_iterator(buf
			+ _vWidth / std::numeric_limits<byte>::digits));
	}

	/**
	* Generates a bitmask with a given number of bits set.
	*/
	template <typename T>
	inline T BitMask(uint32 Count);

	template <>
	inline uint64 BitMask<uint64>(uint32 Count)
	{
		wconstraint(Count <= 64);
		return (uint64(Count < 64) << Count) - 1;
	}

	template <>
	inline uint32 BitMask<uint32>(uint32 Count)
	{
		wconstraint(Count <= 32);
		return uint32(uint64(1) << Count) - 1;
	}

	template <>
	inline uint16 BitMask<uint16>(uint32 Count)
	{
		wconstraint(Count <= 16);
		return uint16((uint32(1) << Count) - 1);
	}

	template <>
	inline uint8 BitMask<uint8>(uint32 Count)
	{
		wconstraint(Count <= 8);
		return uint8((uint32(1) << Count) - 1);
	}
}