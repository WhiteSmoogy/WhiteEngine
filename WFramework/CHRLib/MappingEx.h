/*!	\file MappingEx.h
\ingroup CHRLib
\brief ���ӱ���ӳ�䡣
\version r359
\author FrankHB <frankhb1989@gmail.com>

\par ����ʱ��:
2015-12-29 19:58:36 +0800
\par �޸�ʱ��:
2016-06-19 leohawke port to vs2015
\par �ı�����:
UTF-8
\par ģ������:
CHRLib::MappingEx
*/

#ifndef LBase_CHRLib_MappingEx_h
#define LBase_CHRLib_MappingEx_h 1

#include "WFramework/CHRLib/StaticMapping.hpp"

namespace CHRLib
{

	/*!

	�޶�̬ӳ�䡣
	*/
#define CHRLIB_NODYNAMIC_MAPPING 0

#if CHRLIB_NODYNAMIC_MAPPING
	/*!
	\brief ����ת����
	\since ���� build 132
	*/
	//@{
	wconstexpr const byte cp17[]{ 0 };
	extern "C"
	{
		extern const byte cp113[];
		//extern const byte cp2026[13658];
	}
	wconstexpr const byte cp2026[]{ 0 };
	//@}
#else

	/*!
	\brief ��̬���صı���ת����
	\warning ʹ��ת��ӳ���㷨ǰ���뱣֤�ǿճ�ʼ������������ʧ�ܻ�����δ������Ϊ��
	\warning ���̰߳�ȫ������
	*/
	//@{
	extern byte* cp17;
	extern byte* cp113;
	extern byte* cp2026;
	//@}

#endif

	/*!
	\brief ���ұ���ת�����е�ʵ�֡�
	\note Ĭ��ʹ�ñ���ת����ָ�롣
	*/
	extern ucs2_t(*cp113_lkp)(byte, byte);


	/*!
	\brief �� Unicode ����ӳ��ģ���ػ���
	*/
	//@{
	template<>
	struct GUCSMapper<CharSet::SHIFT_JIS> : UCSMapperBase
	{
		using UCSMapperBase::Assign;

#if 0
		template<typename _tObj, typename _tIn, typename _tState>
		static byte
			Decode(_tObj&& uc, _tIn&& i, _tState&& st) wnoexcept(noexcept(GetSequenceOf(st))
				&& noexcept(GetIndexOf(st)) && noexcept(!FillByte(i, st)))
		{
			uint_least16_t row(0), col(0), ln(188); // (7E-40 + 1 + FC-80 + 1)
			const auto c(FillByte(i, st));

			if ((c >= 0xA1) && (c <= 0xC6))
			{
				const auto d(FillByte(i, st));

				row = c - 0xA1;
				if (d >= 0x40 && d <= 0x7E)
					col = d - 0x40;
				else if (d >= 0xA1 && d <= 0xFE)
					col = d - 0x62;
				Assign(uc, cp17[row * ln + col]);
			}
			else if (c >= 0xC9 && c <= 0xF9)
			{
				const auto d(FillByte(i, st));

				row = c - 0xA3;
				if (d >= 0x40 && d <= 0x7E)
					col = d - 0x40;
				else if (d >= 0xA1 && d <= 0xFE)
					col = d - 0x62;
				Assign(uc, cp17[row * ln + col]);
			}
			else if (c < 0x80)
			{
				Assign(uc, c);
				return 1;
			}
			else
				uc = 0xFFFE;
			return 2;
		}
#endif
	};

	template<>
	struct GUCSMapper<CharSet::GBK> : UCSMapperBase
	{
		using UCSMapperBase::Assign;

		/*!
		\bug ����������ȱ��ת����
		*/
		template<typename _tObj, typename _tIn, typename _tState>
		static ConversionResult
			Decode(_tObj&& uc, _tIn&& i, _tState&& st) wnoexcept(noexcept(GetSequenceOf(st))
				&& noexcept(GetIndexOf(st)) && noexcept(!FillByte(i, st)))
		{
			wassume(cp113_lkp);

			const auto seq(GetSequenceOf(st));

			switch (GetIndexOf(st))
			{
			case 0:
				if WB_UNLIKELY(!FillByte(i, st))
					return ConversionResult::BadSource;
				if (seq[0] < 0x80)
				{
					Assign(uc, seq[0]);
					break;
				}
			case 1:
				if WB_UNLIKELY(!FillByte(i, st))
					return ConversionResult::BadSource;
				{
					const auto t(cp113_lkp(seq[0], seq[1]));

					if WB_LIKELY(t != 0)
					{
						Assign(uc, t);
						break;
					}
				}
				return ConversionResult::Unhandled;
			default:
				return ConversionResult::BadState;
			}
			return ConversionResult::OK;
		}
	};

	template<>
	struct GUCSMapper<CharSet::Big5> : UCSMapperBase
	{
		using UCSMapperBase::Assign;

#if 0
		template<typename _tObj, typename _tIn, typename _tState>
		static byte
			Decode(_tObj&& uc, _tIn&& i, _tState&& st) wnoexcept(noexcept(GetSequenceOf(st))
				&& noexcept(GetIndexOf(st)) && noexcept(!FillByte(i, st)))
		{
			uint_least16_t row(0), col(0), ln(157); // (7E-40 + FE-A1)
			const auto c(FillByte(i, st));

			if (c >= 0xA1 && c <= 0xC6)
			{
				const auto d(FillByte(i, st));

				row = c - 0xA1;
				if (d >= 0x40 && d <= 0x7E)
					col = d - 0x40;
				else if (d >= 0xA1 && d <= 0xFE)
					col = d - 0x62;
				Assign(uc, cp2026[row * ln + col]);
				return 2;
			}
			else if (c >= 0xC9 && c <= 0xF9)
			{
				const auto d(FillByte(i, st));

				row = c - 0xA3;
				if (d >= 0x40 && d <= 0x7E)
					col = c - 0x40;
				else if (d >= 0xA1 && d <= 0xFE)
					col = d - 0x62;
				Assign(uc, cp2026[row * ln + col]);
				return 2;
			}
			else if (c < 0x80)
			{
				Assign(uc, c);
				return 1;
			}
			else
				Assign(uc, 0xFFFE);
			return 2;
		}
#endif
	};
	//@}

	using white::add_pointer_t;
	using white::enable_if_convertible_t;
	//@{
	template<Encoding, typename _tRet, typename... _tParams>
	wconstfn WB_STATELESS white::add_ptr_t<_tRet(_tParams...)>
		FetchMapperPtr_TryUCSMapper(...) wnothrow
	{
		return{};
	}
	template<Encoding _vEnc, typename _tRet, typename... _tParams>
	wconstfn WB_ATTR_returns_nonnull WB_STATELESS white::add_ptr_t<_tRet(_tParams...)>
		FetchMapperPtr_TryUCSMapper(wimpl(enable_if_convertible_t<
			decltype(GUCSMapper<_vEnc>::Decode(std::declval<_tParams>()...)), _tRet>*
			= {})) wnothrow
	{
		return GUCSMapper<_vEnc>::Decode;
	}
	template<Encoding _vEnc, typename _tRet, typename... _tParams>
	wconstfn WB_ATTR_returns_nonnull WB_STATELESS white::add_ptr_t<_tRet(_tParams...)>
		FetchMapperPtr_TryUCSMapper(wimpl(enable_if_convertible_t<
			decltype(GUCSMapper<_vEnc>::Encode(std::declval<_tParams>()...)), _tRet,
			int>* = {})) wnothrow
	{
		return GUCSMapper<_vEnc>::Encode;
	}

	template<typename _tRet, typename... _tParams>
	wconstfn WB_STATELESS white::add_ref_t<_tRet(_tParams...)>
		FetchMapper_Default() wnothrow
	{
		return *FetchMapperPtr_TryUCSMapper<CS_Default, _tRet, _tParams...>(nullptr);
	}

	//! \brief ȡָ������ӳ���ת������ָ�롣
	template<typename _tRet, typename... _tParams>
	wconstfn_relaxed white::add_ptr_t<_tRet(_tParams...)>
		FetchMapperPtr(Encoding enc)
	{
		using namespace CharSet;

#define CHR_MapItem(enc) \
case enc: \
	return FetchMapperPtr_TryUCSMapper<enc, _tRet, _tParams...>(nullptr);

		switch (enc)
		{
			CHR_MapItem(SHIFT_JIS)
				CHR_MapItem(UTF_8)
				CHR_MapItem(GBK)
				CHR_MapItem(UTF_16BE)
				CHR_MapItem(UTF_16LE)
				CHR_MapItem(UTF_32BE)
				CHR_MapItem(UTF_32LE)
				CHR_MapItem(Big5)
		default:
			break;
		}
#undef CHR_MapItem
		return{};
	}
	//@}

} // namespace CHRLib;

#endif
