#ifndef LBase_CHRLib_CharacterProcessing_h
#define LBase_CHRLib_CharacterProcessing_h 1

#include "WBase/cstring.h" // for stdex::ntctslen;
#include "WBase/string.hpp" // for white::string_traits,white::enable_for_string_class_t;

#include "CharacterMapping.h"
#include <cstdio> // for std::FILE;
#include <memory> // for std::move;
#include <algorithm> // for std::copy_n;

namespace CHRLib
{

	using white::enable_for_string_class_t;
	using white::enable_if_t;

	/*!
	\brief �ж����������ַ��� ASCII �ַ�ȡֵ��Χ�ڡ�
	\note ��ȡ�� 7 λ��
	*/
	template<typename _tChar>
	wconstfn bool
		IsASCII(_tChar c)
	{
		return !(c & ~0x7F);
	}

	/*!
	\brief �������������ַ�ת��Ϊ ASCII ȡֵ��Χ���ݵ��ַ���
	\note ��ȡ�� 7 λ��
	*/
	template<typename _tChar>
	wconstfn char
		ToASCII(_tChar c)
	{
		static_assert(std::is_integral<_tChar>(), "Invalid type found.");

		return c & 0x7F;
	}


	//! \return ת�����ֽ�����
	//@{
	/*!
	\brief ��ָ�������ת��״̬ת���ַ����е��ַ�Ϊ UCS-2 �ַ���
	\since build 291
	*/
	//@{
	WB_API ConversionResult
		MBCToUC(ucs2_t&, const char*&, Encoding, ConversionState&& = {});
	//! \since build 614
	WB_API ConversionResult
		MBCToUC(ucs2_t&, const char*&, const char*, Encoding, ConversionState&& = {});
	inline PDefH(ConversionResult, MBCToUC, ucs2_t& uc, const char*& c,
		Encoding enc, ConversionState& st)
		ImplRet(MBCToUC(uc, c, enc, std::move(st)))
		//@}
		/*!
		\brief ��ָ�������ת��״̬ת���ַ����е��ַ�Ϊ UCS-2 �ַ���
		\pre ���ԣ�ָ������ǿա�
		*/
		//@{
		WB_API WB_NONNULL(2) ConversionResult
		MBCToUC(ucs2_t&, std::FILE*, Encoding, ConversionState&& = {});
	inline PDefH(ConversionResult, MBCToUC, ucs2_t& uc, std::FILE* fp, Encoding enc,
		ConversionState& st)
		ImplRet(MBCToUC(uc, fp, enc, std::move(st)))
		//@}
		/*!
		\brief ��ָ�������ת��״̬����ת���ַ�Ϊ UCS-2 �ַ����ֽ�����
		*/
		//@{
		WB_API ConversionResult
		MBCToUC(const char*&, Encoding, ConversionState&& = {});
	//! \since build 614
	WB_API ConversionResult
		MBCToUC(const char*&, const char*, Encoding, ConversionState&& = {});
	inline PDefH(ConversionResult, MBCToUC, const char*& c, Encoding enc,
		ConversionState& st)
		ImplRet(MBCToUC(c, enc, std::move(st)))
		//! \since build 614
		inline PDefH(ConversionResult, MBCToUC, const char*& c, const char* e,
			Encoding enc, ConversionState& st)
		ImplRet(MBCToUC(c, e, enc, std::move(st)))
		//! \pre ���ԣ�ָ������ǿա�
		//@{
		WB_API WB_NONNULL(1) ConversionResult
		MBCToUC(std::FILE*, Encoding, ConversionState&& = {});
	inline WB_NONNULL(1) PDefH(ConversionResult, MBCToUC, std::FILE* fp,
		Encoding enc, ConversionState& st)
		ImplRet(MBCToUC(fp, enc, std::move(st)))
		//@}
		//@}

		/*!
		\brief ��ָ������ת�� UCS-2 �ַ�Ϊ�ַ�����ʾ�Ķ��ֽ��ַ���
		\pre ���ԣ�ָ������ǿ� ��
		\pre ��һ����ָ��Ļ�����������ת�����ַ����С�
		*/
		WB_API WB_NONNULL(1) size_t
		UCToMBC(char*, const ucs2_t&, Encoding);
	//@}


	//! \note �����ֽ���ͬʵ�ֵ� ucs2_t �洢�ֽ���
	//@{
	/*
	\pre ���ԣ�ָ������ǿ� ��
	\pre ��һ����ָ��Ļ�����������ת����� NTCTS ��������β�Ŀ��ַ�����
	\pre ָ�����ָ��Ļ��������ص���
	\return ת���Ĵ�����
	*/
	//@{
	/*!
	\brief ��ָ������ת�� MBCS �ַ���Ϊ UCS-2 �ַ�����
	*/
	//@{
	WB_API WB_NONNULL(1, 2) size_t
		MBCSToUCS2(ucs2_t*, const char*, Encoding = CS_Default);
	//! \since build 614
	WB_API WB_NONNULL(1, 2, 3) size_t
		MBCSToUCS2(ucs2_t*, const char*, const char* e, Encoding = CS_Default);
	//@}

	/*!
	\brief ��ָ������ת�� MBCS �ַ���Ϊ UCS-4 �ַ�����
	*/
	//@{
	WB_API WB_NONNULL(1, 2) size_t
		MBCSToUCS4(ucs4_t*, const char*, Encoding = CS_Default);
	WB_API WB_NONNULL(1, 2, 3) size_t
		MBCSToUCS4(ucs4_t*, const char*, const char*, Encoding = CS_Default);
	//@}

	/*!
	\brief ��ָ������ת�� UCS-2 �ַ���Ϊ MBCS �ַ�����
	*/
	WB_API WB_NONNULL(1, 2) size_t
		UCS2ToMBCS(char*, const ucs2_t*, Encoding = CS_Default);

	/*!
	\brief ת�� UCS-2 �ַ���Ϊ UCS-4 �ַ�����
	*/
	WB_API WB_NONNULL(1, 2) size_t
		UCS2ToUCS4(ucs4_t*, const ucs2_t*);

	/*!
	\brief ��ָ������ת�� UCS-4 �ַ���Ϊ MBCS �ַ�����
	*/
	WB_API WB_NONNULL(1, 2) size_t
		UCS4ToMBCS(char*, const ucs4_t*, Encoding = CS_Default);

	//! \brief ת�� UCS-4 �ַ���Ϊ UCS-2 �ַ�����
	WB_API WB_NONNULL(1, 2) size_t
		UCS4ToUCS2(ucs2_t*, const ucs4_t*);
	//@}


	/*!
	\pre �����ַ�����ÿ���ַ������� <tt>sizeof(ucsint_t)</tt> �ֽڡ�
	\pre ���ԣ�ָ������ǿա�
	*/
	//@{
	//! \brief ת��ָ������Ķ��ֽ��ַ���Ϊָ�����͵� UCS-2 �ַ�����
	//@{
	template<class _tDst = std::basic_string<ucs2_t>>
	WB_NONNULL(1) _tDst
		MakeUCS2LE(const char* s, typename _tDst::size_type n,
			Encoding enc = CS_Default)
	{
		wconstraint(s);

		_tDst str(n, typename white::string_traits<_tDst>::value_type());

		str.resize(MBCSToUCS2(&str[0], s, enc));
		return str;
	}
	template<class _tDst = std::basic_string<ucs2_t>>
	WB_NONNULL(1) _tDst
		MakeUCS2LE(const char* s, Encoding enc = CS_Default)
	{
		return MakeUCS2LE<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	//! \brief ����ָ�����͵� UCS-2 �ַ�����
	//@{
	//! \since build 594
	template<class _tDst = std::basic_string<ucs2_t>>
	WB_NONNULL(1) inline _tDst
		MakeUCS2LE(const ucs2_t* s, typename _tDst::size_type,
			Encoding = CharSet::ISO_10646_UCS_2)
	{
		wconstraint(s);

		// FIXME: Correct conversion for encoding other than UCS-2LE.
		return s;
	}
	template<class _tDst = std::basic_string<ucs2_t>>
	WB_NONNULL(1) inline _tDst
		MakeUCS2LE(const ucs2_t* s, Encoding enc = CharSet::ISO_10646_UCS_2)
	{
		return MakeUCS2LE<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	//! \brief ת�� UCS-4 �ַ���Ϊָ�����͵� UCS-2 �ַ�����
	//@{
	//! \since build 594
	template<class _tDst = std::basic_string<ucs2_t>>
	WB_NONNULL(1) _tDst
		MakeUCS2LE(const ucs4_t* s, typename _tDst::size_type n,
			Encoding = CharSet::ISO_10646_UCS_4)
	{
		wconstraint(s);

		_tDst str(n, typename white::string_traits<_tDst>::value_type());

		str.resize(UCS4ToUCS2(&str[0], s));
		return str;
	}
	template<class _tDst = std::basic_string<ucs2_t>>
	WB_NONNULL(1) _tDst
		MakeUCS2LE(const ucs4_t* s, Encoding enc = CharSet::ISO_10646_UCS_4)
	{
		return MakeUCS2LE<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	//! \note ת��ָ�����͵� UCS2-LE �ַ���������Դ���Ͳ�������ֱ�ӹ���Ŀ������ʱ��Ч��
	template<class _tString, class _tDst = std::basic_string<ucs2_t>,
		wimpl(typename = enable_for_string_class_t<_tString>, typename
			= enable_if_t<!std::is_constructible<_tDst, _tString>::value>)>
		inline _tDst
		MakeUCS2LE(const _tString& str, Encoding enc = CS_Default)
	{
		return CHRLib::MakeUCS2LE<_tDst>(str.data(), str.length(), enc);
	}
	//! \note ����ָ�����͵� UCS2-LE �ַ���������Դ���Ͳ�����ֱ�ӹ���Ŀ������ʱ��Ч��
	template<class _tString, class _tDst = std::basic_string<ucs2_t>,
		wimpl(typename = enable_for_string_class_t<_tString>, typename
			= enable_if_t<std::is_constructible<_tDst, _tString>::value>)>
		inline _tDst
		MakeUCS2LE(_tString&& str)
	{
		return std::forward<_tDst>(str);
	}

	/*!
	\pre �����ַ�����ÿ���ַ������� <tt>sizeof(ucsint_t)</tt> �ֽڡ�
	\pre ��Ӷ��ԣ�ָ������ǿա�
	\since build 594
	*/
	//@{
	//! \brief ת��ָ������Ķ��ֽ��ַ���Ϊָ�����͵� UCS-4 �ַ�����
	//@{
	template<class _tDst = std::basic_string<ucs4_t>>
	WB_NONNULL(1) _tDst
		MakeUCS4LE(const char* s, typename _tDst::size_type n,
			Encoding enc = CS_Default)
	{
		_tDst str(n, typename white::string_traits<_tDst>::value_type());

		str.resize(MBCSToUCS4(&str[0], s, enc));
		return str;
	}
	template<class _tDst = std::basic_string<ucs4_t>>
	WB_NONNULL(1) _tDst
		MakeUCS4LE(const char* s, Encoding enc = CS_Default)
	{
		return MakeUCS4LE<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	//! \brief ����ָ�����͵� UCS-4 �ַ�����
	//@{
	template<class _tDst = std::basic_string<ucs4_t>>
	WB_NONNULL(1) inline _tDst
		MakeUCS4LE(const ucs4_t* s, typename _tDst::size_type,
			Encoding = CharSet::ISO_10646_UCS_4)
	{
		wconstraint(s);

		// FIXME: Correct conversion for encoding other than UCS-4LE.
		return s;
	}
	template<class _tDst = std::basic_string<ucs4_t>>
	WB_NONNULL(1) inline _tDst
		MakeUCS4LE(const ucs4_t* s, Encoding enc = CharSet::ISO_10646_UCS_4)
	{
		return MakeUCS4LE<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	//! \brief ת�� UCS-2 �ַ���Ϊָ�����͵� UCS-4 �ַ�����
	//@{
	template<class _tDst = std::basic_string<ucs4_t>>
	WB_NONNULL(1) _tDst
		MakeUCS4LE(const ucs2_t* s, typename _tDst::size_type n,
			Encoding = CharSet::ISO_10646_UCS_2)
	{
		_tDst str(n, typename white::string_traits<_tDst>::value_type());

		str.resize(UCS2ToUCS4(&str[0], s));
		return str;
	}
	template<class _tDst = std::basic_string<ucs4_t>>
	WB_NONNULL(1) _tDst
		MakeUCS4LE(const ucs2_t* s, Encoding enc = CharSet::ISO_10646_UCS_2)
	{
		return MakeUCS4LE<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	//! \note ת��ָ�����͵� UCS4-LE �ַ���������Դ���Ͳ�������ֱ�ӹ���Ŀ������ʱ��Ч��
	template<class _tString, class _tDst = std::basic_string<ucs4_t>,
		wimpl(typename = enable_for_string_class_t<_tString>, typename
			= enable_if_t<!std::is_constructible<_tDst, _tString>::value>)>
		inline _tDst
		MakeUCS4LE(const _tString& str, Encoding enc = CS_Default)
	{
		return CHRLib::MakeUCS4LE<_tDst>(str.c_str(), str.length(), enc);
	}
	//! \note ����ָ�����͵� UCS4-LE �ַ���������Դ���Ͳ�����ֱ�ӹ���Ŀ������ʱ��Ч��
	template<class _tString, class _tDst = std::basic_string<ucs4_t>,
		wimpl(typename = enable_for_string_class_t<_tString>, typename
			= enable_if_t<std::is_constructible<_tDst, _tString>::value>)>
		inline _tDst
		MakeUCS4LE(_tString&& str)
	{
		return std::forward<_tDst>(str);
	}

	//! \brief ������ֽ��ַ�����
	//@{
	template<class _tDst = std::string>
	inline WB_NONNULL(1) _tDst
		MakeMBCS(const char* s)
	{
		wconstraint(s);

		return _tDst(s);
	}
	//! \since build 594
	template<class _tDst = std::string>
	inline WB_NONNULL(1) _tDst
		MakeMBCS(const char* s, typename _tDst::size_type n)
	{
		return _tDst(s, n);
	}
	//@}
	//! \brief ת�� UTF-8 �ַ���Ϊָ������Ķ��ֽ��ַ�����
	//@{
	template<class _tDst = std::string>
	inline WB_NONNULL(1) _tDst
		MakeMBCS(const char* s, Encoding enc)
	{
		return enc = CS_Default ? MakeMBCS<_tDst>(s)
			: MakeMBCS<_tDst>(MakeUCS2LE(s, CS_Default), enc);
	}
	//! \since build 594
	template<class _tDst = std::string>
	inline WB_NONNULL(1) _tDst
		MakeMBCS(const char* s, typename _tDst::size_type n, Encoding enc)
	{
		return enc = CS_Default ? MakeMBCS<_tDst>(s, n)
			: MakeMBCS<_tDst>(MakeUCS2LE(s, CS_Default), enc);
	}
	//@}
	//! \brief ת�� UCS-2LE �ַ���Ϊָ������Ķ��ֽ��ַ�����
	//@{
	//! \since build 594
	template<class _tDst = std::string>
	WB_NONNULL(1) _tDst
		MakeMBCS(const ucs2_t* s, typename _tDst::size_type n,
			Encoding enc = CS_Default)
	{
		wconstraint(s);

		const auto w(FetchMaxCharWidth(enc));
		_tDst str(n * (w == 0 ? sizeof(ucsint_t) : w),
			typename white::string_traits<_tDst>::value_type());

		str.resize(UCS2ToMBCS(&str[0], s, enc));
		return str;
	}
	template<class _tDst = std::string>
	WB_NONNULL(1) _tDst
		MakeMBCS(const ucs2_t* s, Encoding enc = CS_Default)
	{
		return MakeMBCS<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	/*!
	\brief ת�� UCS-4LE �ַ���Ϊָ������Ķ��ֽ��ַ�����
	\since build 594
	*/
	//@{
	template<class _tDst = std::string>
	WB_NONNULL(1) _tDst
		MakeMBCS(const ucs4_t* s, typename _tDst::size_type n,
			Encoding enc = CS_Default)
	{
		wconstraint(s);

		_tDst str(n * FetchMaxCharWidth(enc),
			typename white::string_traits<_tDst>::value_type());

		str.resize(UCS4ToMBCS(&str[0], s, enc));
		return str;
	}
	template<class _tDst = std::string>
	WB_NONNULL(1) _tDst
		MakeMBCS(const ucs4_t* s, Encoding enc = CS_Default)
	{
		return MakeMBCS<_tDst>(s, white::ntctslen(s), enc);
	}
	//@}
	//! \note ת��ָ�����͵Ķ��ֽ��ַ���������Դ���Ͳ�������ֱ�ӹ���Ŀ������ʱ��Ч��
	template<class _tDst = std::basic_string<char>,
	class _tString = std::basic_string<ucs2_t>,
		wimpl(typename = enable_for_string_class_t<_tString>, typename
			= enable_if_t<!std::is_constructible<_tDst, _tString>::value>)>
		inline _tDst
		MakeMBCS(const _tString& str, Encoding enc = CS_Default)
	{
		return CHRLib::MakeMBCS<_tDst>(str.c_str(), str.length(), enc);
	}
	//! \note ����ָ�����͵Ķ��ֽ��ַ���������Դ���Ͳ�����ֱ�ӹ���Ŀ������ʱ��Ч��
	template<class _tString, class _tDst = std::basic_string<char>,
		wimpl(typename = enable_for_string_class_t<_tString>, typename
			= enable_if_t<std::is_constructible<_tDst, _tString>::value>)>
		inline _tDst
		MakeMBCS(_tString&& str)
	{
		return std::forward<_tDst>(str);
	}
	//@}

} // namespace CHRLib;

#endif
