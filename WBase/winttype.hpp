/*
\par �޸�ʱ��:
2016-11-27 00:56 +0800
*/

#ifndef WBase_winttype_hpp
#define WBase_winttype_hpp 1

#include "wdef.h"
#include <numeric>
#include <limits>

namespace white
{
	/*!
	\brief �ֽ���
	\todo ʹ�õ�����ͷ�ļ���
	*/
	enum class byte_order
	{
		unknown = 0,
		neutral = 1,
		little = 2,
		big = 3,
		PDP = 4
	};

	//! \since build CPP17
	//@{
	namespace details
	{
		using byte = stdex::byte;

		struct bit_order_tester
		{
			unsigned char le : 4, : CHAR_BIT - 4;
		};

		union byte_order_tester
		{
			std::uint_least32_t n;
			byte p[4];
		};

	} // namespace details;

	  //! \brief ���Ա����ֽ���
	wconstfn_relaxed WB_STATELESS byte_order
		native_byte_order()
	{
		wconstexpr const details::byte_order_tester x = { 0x01020304 };

		return x.p[0] == 4 ? byte_order::little : (x.p[0] == 1 ? byte_order::big
			: (x.p[0] == 2 ? byte_order::PDP : byte_order::unknown));
	}

	//! \brief ���Ա���λ��
	wconstfn WB_STATELESS bool
		native_little_bit_order()
	{
		return bool(details::bit_order_tester{ 1 }.le & 1);
	}
	//@}

	inline namespace inttype {
		using stdex::byte;
		using stdex::octet;
		using uint8 = std::uint8_t;
		using int8 = std::int8_t;
		using uint16 = std::uint16_t;
		using int16 = std::int16_t;
		using uint32 = std::uint32_t;
		using int32 = std::int32_t;
		using uint64 = std::uint64_t;
		using int64 = std::int64_t;
	}

	using std::integral_constant;
	using std::make_signed;
	using std::make_unsigned;
	
	template<typename _tInt>
	//ȡ�������͵�λ���
	struct integer_width : integral_constant<size_t, sizeof(_tInt) * CHAR_BIT>
	{};

	template<typename _type, bool>
	//�з�������
	struct make_signed_c : make_signed<_type>
	{};

	template<typename _type>
	//�޷�������
	struct make_signed_c<_type, false> : make_unsigned<_type>
	{};

	template<size_t _vWidth>
	//ȡָ����ȵ���������
	struct make_width_int
	{
		static_assert(_vWidth <= 64, "Width too large found.");

		using fast_type = typename make_width_int<(_vWidth <= 8U ? 8U
			: (_vWidth <= 16U ? 16U : (_vWidth <= 32U ? 32U : 64U)))>::fast_type;
		using unsigned_fast_type = typename make_width_int<(_vWidth <= 8U ? 8U
			: (_vWidth <= 16U ? 16U : (_vWidth <= 32U ? 32U : 64U)))>
			::unsigned_fast_type;
		using least_type = typename make_width_int<(_vWidth <= 8U ? 8U
			: (_vWidth <= 16U ? 16U : (_vWidth <= 32U ? 32U : 64U)))>::least_type;
		using unsigned_least_type = typename make_width_int<(_vWidth <= 8U ? 8U
			: (_vWidth <= 16U ? 16U : (_vWidth <= 32U ? 32U : 64U)))>
			::unsigned_least_type;
	};

	template<>
	struct make_width_int<8U>
	{
		using type = int8;
		using unsigned_type = uint8;
		using fast_type = std::int_fast8_t;
		using unsigned_fast_type = std::uint_fast8_t;
		using least_type = std::int_least8_t;
		using unsigned_least_type = std::uint_least8_t;
	};

	template<>
	struct make_width_int<16U>
	{
		using type = int16;
		using unsigned_type = uint16;
		using fast_type = std::int_fast16_t;
		using unsigned_fast_type = std::uint_fast16_t;
		using least_type = std::int_least16_t;
		using unsigned_least_type = std::uint_least16_t;
	};

	template<>
	struct make_width_int<32U>
	{
		using type = int32;
		using unsigned_type = uint32;
		using fast_type = std::int_fast32_t;
		using unsigned_fast_type = std::uint_fast32_t;
		using least_type = std::int_least32_t;
		using unsigned_least_type = std::uint_least32_t;
	};

	template<>
	struct make_width_int<64U>
	{
		using type = int64;
		using unsigned_type = uint64;
		using fast_type = std::int_fast64_t;
		using unsigned_fast_type = std::uint_fast64_t;
		using least_type = std::int_least64_t;
		using unsigned_least_type = std::uint_least64_t;
	};

	/**
	* Aligns a value to the nearest higher multiple of 'Alignment', which must be a power of two.
	*
	* @param  Val        The value to align.
	* @param  Alignment  The alignment value, must be a power of two.
	*
	* @return The value aligned up to the specified alignment.
	*/
	template <typename T>
	inline constexpr T Align(T Val, uint64 Alignment)
	{
		return (T)(((uint64)Val + Alignment - 1) & ~(Alignment - 1));
	}

	template <typename T>
	inline constexpr T AlignDown(T Val, uint64 Alignment)
	{
		return (T)(((uint64)Val) & ~(Alignment - 1));
	}

	/**
	 * Aligns a value to the nearest higher multiple of 'Alignment'.
	 *
	 * @param  Val        The value to align.
	* @param  Alignment  The alignment value, can be any arbitrary value.
	 *
	* @return The value aligned up to the specified alignment.
	*/
	template <typename T>
	requires std::is_integral_v<T> || std::is_pointer_v<T>
	inline constexpr T AlignArbitrary(T Val, uint64 Alignment)
	{
		return (T)((((uint64)Val + Alignment - 1) / Alignment) * Alignment);
	}
}

#endif