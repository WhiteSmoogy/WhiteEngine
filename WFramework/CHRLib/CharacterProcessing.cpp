/*!	\file CharacterProcessing.cpp
\ingroup CHRLib
\brief �ַ����봦��
\version r1634
\author FrankHB <frankhb1989@gmail.com>
\par ����ʱ��:
2015-11-28 17:53:21 +0800
\par �޸�ʱ��:

\par �ı�����:
UTF-8
\par ģ������:
CHRLib::CharacterProcessing
*/

#include "WBase/algorithm.hpp"
#include "CharacterProcessing.h"
#include "MappingEx.h"
#include "Convert.hpp"

#include <cctype>
#include <cstdlib>
#include <cwchar>

namespace CHRLib
{

	using std::malloc;
	using std::size_t;
	using std::tolower;
	using white::is_null;
	using white::ntctslen;
	using std::make_unique;

	ConversionResult
		MBCToUC(char16_t& uc, const char*& c, Encoding enc, ConversionState&& st)
	{
		if (const auto pfun = FetchMapperPtr<ConversionResult, char16_t&, const char*&,
			ConversionState&&>(enc))
			return pfun(uc, c, std::move(st));
		return ConversionResult::Unhandled;
	}
	ConversionResult
		MBCToUC(char16_t& uc, const char*& c, const char* e, Encoding enc,
			ConversionState&& st)
	{
		wconstraint(c <= e);
		if (const auto pfun = FetchMapperPtr<ConversionResult, char16_t&,
			GuardPair<const char*>&&, ConversionState&&>(enc))
			return pfun(uc, { c, e }, std::move(st));
		return ConversionResult::Unhandled;
	}
	ConversionResult
		MBCToUC(char16_t& uc, std::FILE* fp, Encoding enc, ConversionState&& st)
	{
		wconstraint(fp);
		if (const auto pfun = FetchMapperPtr<ConversionResult, char16_t&,
			white::ifile_iterator&, ConversionState&&>(enc))
		{
			white::ifile_iterator i(fp);
			const auto r(pfun(uc, i, std::move(st)));

			i.sungetc(fp);
			return r;
		}
		return ConversionResult::Unhandled;
	}
	ConversionResult
		MBCToUC(const char*& c, Encoding enc, ConversionState&& st)
	{
		if (const auto pfun = FetchMapperPtr<ConversionResult,
			white::pseudo_output&&, const char*&, ConversionState&&>(enc))
			return pfun(white::pseudo_output(), c, std::move(st));
		return ConversionResult::Unhandled;
	}
	ConversionResult
		MBCToUC(const char*& c, const char* e, Encoding enc, ConversionState&& st)
	{
		wconstraint(c <= e);
		if (const auto pfun = FetchMapperPtr<ConversionResult,
			white::pseudo_output&&, GuardPair<const char*>&&,
			ConversionState&&>(enc))
			return pfun(white::pseudo_output(), { c, e }, std::move(st));
		return ConversionResult::Unhandled;
	}
	ConversionResult
		MBCToUC(std::FILE* fp, Encoding enc, ConversionState&& st)
	{
		wconstraint(fp);
		if (const auto pfun = FetchMapperPtr<ConversionResult,
			white::pseudo_output&&, white::ifile_iterator&,
			ConversionState&&>(enc))
		{
			white::ifile_iterator i(fp);
			const auto r(pfun(white::pseudo_output(), i, std::move(st)));

			i.sungetc(fp);
			return r;
		}
		return ConversionResult::Unhandled;
	}

	size_t
		UCToMBC(char* d, const char16_t& s, Encoding enc)
	{
		wconstraint(d);

		size_t l(0);

		if (const auto pfun = FetchMapperPtr<size_t, char*, char32_t>(enc))
			l = pfun(d, s);
		return l;
	}


	size_t
		MBCSToUCS2(ucs2_t* d, const char* s, Encoding enc)
	{
		wconstraint(d),
		wconstraint(s);

		const auto p(d);

		if (const auto pfun = FetchMapperPtr<ConversionResult, ucs2_t&,
			const char*&, ConversionState&&>(enc))
			while (!is_null(*s) && pfun(*d, s, ConversionState())
				== ConversionResult::OK)
				++d;
		*d = 0;
		return size_t(d - p);
	}
	size_t
		MBCSToUCS2(ucs2_t* d, const char* s, const char* e, Encoding enc)
	{
		wconstraint(d),
		wconstraint(s),
		wconstraint(e),
		wconstraint(s <= e);

		const auto p(d);

		if (const auto pfun = FetchMapperPtr<ConversionResult, ucs2_t&,
			GuardPair<const char*>&&, ConversionState&&>(enc))
			while (!is_null(*s) && pfun(*d, { s, e }, ConversionState())
				== ConversionResult::OK)
				++d;
		*d = 0;
		return size_t(d - p);
	}


	size_t
		MBCSToUCS4(ucs4_t* d, const char* s, Encoding enc)
	{
		wconstraint(d),
			wconstraint(s);

		const auto p(d);

		// TODO: Use UCS-4 internal conversion directly?
		if (const auto pfun = FetchMapperPtr<ConversionResult, ucs2_t&,
			const char*&, ConversionState&&>(enc))
			while (!is_null(*s))
			{
				// TODO: Necessary initialization?
				ucs2_t c;

				if (pfun(c, s, ConversionState()) == ConversionResult::OK)
				{
					*d = c;
					++d;
				}
				else
					break;
			}
		*d = 0;
		return size_t(d - p);
	}
	size_t
		MBCSToUCS4(ucs4_t* d, const char* s, const char* e, Encoding enc)
	{
		wconstraint(d),
			wconstraint(s),
			wconstraint(e),
			wconstraint(s <= e);

		const auto p(d);

		// TODO: Use UCS-4 internal conversion directly?
		if (const auto pfun = FetchMapperPtr<ConversionResult, ucs2_t&,
			GuardPair<const char*>&&, ConversionState&&>(enc))
			while (!is_null(*s))
			{
				// TODO: Necessary initialization?
				ucs2_t c;

				if (pfun(c, { s, e }, ConversionState()) == ConversionResult::OK)
				{
					*d = c;
					++d;
				}
				else
					break;
			}
		*d = 0;
		return size_t(d - p);
	}

	size_t
		UCS2ToMBCS(char* d, const ucs2_t* s, Encoding enc)
	{
		wconstraint(d),
			wconstraint(s);

		const auto p(d);

		if (const auto pfun = FetchMapperPtr<size_t, char*, ucs4_t>(enc))
			while (!is_null(*s))
				d += pfun(d, *s++);
		*d = char();
		return size_t(d - p);
	}

	size_t
		UCS2ToUCS4(ucs4_t* d, const ucs2_t* s)
	{
		const auto p(white::copy_when(s, d, [](ucs2_t c) wnothrow{
			return !is_null(c);
		}));

		*p = ucs4_t();
		return size_t(p - d);
	}

	size_t
		UCS4ToMBCS(char* d, const ucs4_t* s, Encoding enc)
	{
		wconstraint(d),
			wconstraint(s);

		const auto p(d);

		if (const auto pfun = FetchMapperPtr<size_t, char*, ucs4_t>(enc))
			while (!is_null(*s))
				d += pfun(d, ucs2_t(*s++));
		*d = char();
		return size_t(d - p);
	}

	size_t
		UCS4ToUCS2(ucs2_t* d, const ucs4_t* s)
	{
		const auto p(white::transform_when(s, d, [](ucs4_t c) wnothrow{
			return !is_null(c);
		}, [](ucs4_t c) wnothrow{
			return ucs2_t(c);
		}));

		*p = ucs2_t();
		return size_t(p - d);
	}

} // namespace CHRLib;