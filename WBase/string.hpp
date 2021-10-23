/*!	\file string.hpp
\ingroup WBase
\brief ISO C++ ��׼�ַ�����չ��
*/

#ifndef WBase_string_hpp
#define WBase_string_hpp 1

#include "memory.hpp" // for decay_t, remove_rcv_t, std::declval,
//	nested_allocator, is_enum, is_class;
#include "container.hpp" // for "container.hpp", make_index_sequence,
//	index_sequence, begin, end, sort_unique, size, underlying;
#include "cstdio.h" // for wconstraint, vfmtlen;
#include "cstring.h" // for ntctslen;
#include "string_utility.hpp"
#include "array.hpp" // for std::bidirectional_iterator_tag, to_array;
#include "type_traits.hpp"
#include <istream> // for std::basic_istream;
#include "ios.hpp" // for rethrow_badstate;
#include <ostream> // for std::basic_ostream;
#include <sstream> // for std::ostringstream, std::wostring_stream;
#include <cstdarg> // for std::va_list;

namespace white
{
	


	//! \since build 1.3
	namespace details
	{

		//! \since build 1.4
		//@{
		template<typename _type, typename = void>
		struct is_string_like : false_type
		{};

		template<typename _type>
		struct is_string_like<_type, enable_if_t<
			is_object<decay_t<decltype(std::declval<_type>()[0])>>::value>> : true_type
		{};
		//@}


		//! \todo ֧�� std::forward_iterator_tag ���ء�
		template<typename _tFwd1, typename _tFwd2, typename _fPred>
		bool
			ends_with_iter_dispatch(_tFwd1 b, _tFwd1 e, _tFwd2 bt, _tFwd2 et,
				_fPred comp, std::bidirectional_iterator_tag)
		{
			auto i(e);
			auto it(et);

			while (i != b && it != bt)
				if (!comp(*--i, *--it))
					return{};
			return it == bt;
		}


		//! \since build 1.4
		//@{
		template<size_t>
		struct str_algos;

		template<>
		struct str_algos<0>
		{
			//! \since build 1.4
			template<class _tString,
				typename _tSize = typename string_traits<_tString>::size_type>
				static wconstfn auto
				erase_left(_tString& s, _tSize n) wnothrowv
				->wimpl(decltype(s.remove_prefix(n), s))
			{
				return wconstraint(n < s.size() || n == _tSize(-1)),
					s.remove_prefix(n != _tSize(-1) ? n : _tSize(s.size())), s;
			}

			//! \since build 1.4
			template<class _tString,
				typename _tSize = typename string_traits<_tString>::size_type>
				static wconstfn auto
				erase_right(_tString& s, _tSize n) wnothrowv
				->wimpl(decltype(s.remove_suffix(n), s))
			{
				return wconstraint(n < s.size() || n == _tSize(-1)),
					s.remove_suffix(s.size() - n - 1), s;
			}
		};

		template<>
		struct str_algos<1>
		{
			//! \since build 1.4
			template<class _tString,
				typename _tSize = typename string_traits<_tString>::size_type>
				static wconstfn auto
				erase_left(_tString& s, _tSize n) wnothrowv
				->wimpl(decltype(s.erase(0, n)))
			{
				return wconstraint(n < s.size() || n == _tSize(-1)), s.erase(0, n);
			}

			//! \since build 1.4
			template<class _tString,
				typename _tSize = typename string_traits<_tString>::size_type>
				static wconstfn auto
				erase_right(_tString& s, _tSize n) wnothrowv
				->wimpl(decltype(s.erase(n + 1)))
			{
				return wconstraint(n < s.size() || n == _tSize(-1)), s.erase(n + 1);
			}
		};


		template<wimpl(typename = make_index_sequence<2>)>
		struct str_algo;

		template<>
		struct str_algo<index_sequence<>>
		{
			void
				erase_left() = delete;

			void
				erase_right() = delete;
		};

		template<size_t _vIdx, size_t... _vSeq>
		struct str_algo<index_sequence<_vIdx, _vSeq...>>
			: str_algos<_vIdx>, str_algo<index_sequence<_vSeq...>>
		{
			using str_algos<_vIdx>::erase_left;
			using str_algo<index_sequence<_vSeq...>>::erase_left;
			using str_algos<_vIdx>::erase_right;
			using str_algo<index_sequence<_vSeq...>>::erase_right;
		};
		//@}

	} // unnamed namespace;


	  //! \ingroup unary_type_traits
	  //@{
	  /*!
	  \brief �ж�ָ�������Ƿ�Ϊ���ַ������͡�
	  \since build 1.4
	  */
	template<typename _type>
	struct is_string_like : details::is_string_like<_type>
	{};

	/*!
	\brief �ж�ָ�������Ƿ�Ϊ�ַ��������͡�
	\since build 1.4
	*/
	template<typename _type>
	struct is_string_class : and_<is_class<_type>, is_string_like<_type>>
	{};
	//@}

	/*!
	\ingroup metafunctions
	\brief ѡ���ַ��������͵��ض����ر�����������ַ������ͳ�ͻ��
	\sa enable_if_t
	\since build 1.4
	*/
	template<typename _tParam, typename _type = void>
	using enable_for_string_class_t
		= enable_if_t<is_string_class<decay_t<_tParam>>::value, _type>;

	

	/*!	\defgroup string_algorithms String Algorithms
	\addtogroup algorithms
	\brief �ַ����㷨��
	\note ģ���βιؼ��� \c class ��ʾ��֧�������Ͷ����ַ�����
	*/
	//@{
	/*!
	\breif �����ַ������ȡ�
	*/
	//@{
	template<typename _type>
	inline size_t
		string_length(const _type* str) wnothrow
	{
		return std::char_traits<_type>::length(str);
	}
	template<class _type>
	wconstfn auto
		//�ַ�������
		string_length(const _type& str)->decltype(std::size(str))
	{
		return std::size(str);
	}
#if __cplusplus <= 201402L
	//! \see http://wg21.cmeerw.net/cwg/issue1591 ��
	template<typename _tElem>
	wconstfn size_t
		string_length(std::initializer_list<_tElem> il)
	{
		return il.size();
	}
#endif
	//@}


	/*!
	\note ʹ�� ADL string_begin �� string_end ָ����Χ��������
	\note �� ADL ��ӿ�ͬ Boost.StringAlgo ��
	\sa white::string_begin, white::string_end
	\since build 1.4
	*/
	//@{
	//! \brief �жϵ�һ������ָ���Ĵ��Ƿ��Եڶ���������ʼ��
	//@{
	//! \since build 519
	template<typename _tRange1, typename _tRange2, typename _fPred>
	bool
		begins_with(const _tRange1& input, const _tRange2& test, _fPred comp)
	{
		return white::string_length(test) <= white::string_length(input)
			&& std::equal(string_begin(test), string_end(test),
				string_begin(input), comp);
	}
	//! \since build 1.4
	template<typename _tRange1, typename _tRange2>
	inline bool
		begins_with(const _tRange1& input, const _tRange2& test)
	{
		return white::begins_with(input, test, is_equal());
	}
	//@}

	//! \brief �жϵ�һ������ָ���Ĵ��Ƿ��Եڶ�����������
	//@{
	template<typename _tRange1, typename _tRange2, typename _fPred>
	inline bool
		ends_with(const _tRange1& input, const _tRange2& test, _fPred comp)
	{
		// NOTE: See $2014-07 @ %Documentation::Workflow::Annual2014.
		return details::ends_with_iter_dispatch(string_begin(input),
			string_end(input), string_begin(test), string_end(test), comp, typename
			std::iterator_traits<remove_reference_t<decltype(string_begin(input))>>
			::iterator_category());
	}
	template<typename _tRange1, typename _tRange2>
	inline bool
		ends_with(const _tRange1& input, const _tRange2& test)
	{
		return white::ends_with(input, test, is_equal());
	}
	//@}
	//@}

	/*!
	\brief �ж��Ƿ�����Ӵ���
	\since build 1.4
	\todo ������������Ӵ����ذ汾���Ż������⹹���Ӵ�����
	*/
	//@{
	template<class _tString, typename _type>
	inline bool
		exists_substr(const _tString& str, const _type& sub)
	{
		return str.find(sub) != _tString::npos;
	}
	template<class _tString, typename _type>
	inline bool
		exists_substr(const _tString& str, const typename _tString::value_type* p_sub)
	{
		wconstraint(p_sub);

		return str.find(p_sub) != _tString::npos;
	}
	//@}


	//! \ingroup NTCTSUtil
	//@{
	/*!
	\brief NTCTS ���滯���������ַ�֮ǰ���ַ���
	\post <tt>str.length() == white::ntctslen(str.c_str())</tt> ��
	\since build 606
	*/
	template<class _tString>
	auto
		normalize(_tString& str) -> decltype(str.resize(white::ntctslen(str.c_str())))
	{
		return str.resize(white::ntctslen(str.c_str()));
	}

	/*!
	\brief ���Ʋ�����ָ�����ȵ� NTCTS ��
	\note Ŀ���ַ�������ָ�����ȵĲ��ֿ��ܱ������ַ���
	\warning Դ�ַ�����ָ��������û�п��ַ���Ŀ���ַ�������֤�Կ��ַ���β��
	\since build 604
	*/
	//@{
	//! \pre ���ԣ�ָ������ǿա�
	template<class _tString,
		wimpl(typename = white::enable_for_string_class_t<_tString>)>
		inline _tString&&
		ntctsncpy(_tString&& str, const typename string_traits<_tString>::const_pointer
			s, const typename string_traits<_tString>::size_type n)
	{
		wconstraint(s);
		return static_cast<_tString&&>(str = decay_t<_tString>(s, n));
	}
	template<class _tString,
		wimpl(typename = white::enable_for_string_class_t<_tString>)>
		inline _tString&&
		ntctsncpy(_tString&& str, _tString&& s,
			const typename string_traits<_tString>::size_type n)
	{
		auto sub(wforward(s));

		sub.resize(n);
		return static_cast<_tString&&>(str = std::move(sub));
	}
	//@}
	//@}


	/*!
	\brief ���Ƴ��Ȳ�����ָ��������
	\since build 1.4
	*/
	template<class _tString>
	inline _tString&&
		restrict_length(_tString&& str,
			const typename string_traits<_tString>::size_type n)
	{
		if (n < str.length())
			str.resize(n);
		return static_cast<_tString&&>(str);
	}

	/*!
	\brief ȡ��ĸ��������ַ����Ĳ��ظ�����
	*/
	template<class _tString>
	_tString
		alph(_tString& str)
	{
		_tString res(str);

		white::sort_unique(res);
		return res;
	}

	/*!
	\brief �ظ����ӡ�
	\param str �����ӵ��ַ��������á�
	\param n ���ӽ������ԭ�ַ������ظ�������
	\pre ���ԣ� <tt>1 < n</tt> ��
	\todo ��� reserve ��
	*/
	template<class _tString>
	void
		concat(_tString& str, size_t n)
	{
		wconstraint(n != 0);
		if (1 < n)
		{
			const auto len(str.length());

			//����˹ũ���㷨
			white::concat(str, n / 2);
			str.append(str.data(), str.length());
			if (n % 2 != 0)
				str.append(str.data(), len);
		}
	}

	//!\pre �ַ����������� std::basic_string �� basic_string_view �Ĳ������쳣�淶��
	//@{
	//! \since build 1.4
	//@{
	//! \brief ɾ���ַ�����ָ���������ַ������ַ�����
	//@{
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_left(typename string_traits<_tString>::size_type pos, _tString&& str)
	{
		return static_cast<_tString&&>(pos != decay_t<_tString>::npos
			? details::str_algo<>::erase_left(str, pos) : str);
	}
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_left(_tString&& str, typename string_traits<_tString>::value_type c)
	{
		return static_cast<_tString&&>(white::erase_left(str.find_last_of(c), str));
	}
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_left(_tString&& str, const remove_reference_t<_tString>& t)
	{
		return static_cast<_tString&&>(white::erase_left(str.find_last_of(t), str));
	}
	/*!
	\pre ���ԣ�ָ������ǿա�
	\since build 1.4
	*/
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_left(_tString&& str, typename string_traits<_tString>::const_pointer t
			= &to_array<typename string_traits<_tString>::value_type>(" \f\n\r\t\v")[0])
	{
		wconstraint(t);
		return static_cast<_tString&&>(white::erase_left(str.find_last_of(t), str));
	}
	//@}

	//! \brief ɾ���ַ�����ָ���������ַ��Ҳ���ַ�����
	//@{
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_right(typename string_traits<_tString>::size_type pos, _tString&& str)
	{
		return static_cast<_tString&&>(pos != decay_t<_tString>::npos
			? details::str_algo<>::erase_right(str, pos) : str);
	}
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_right(_tString&& str, typename string_traits<_tString>::value_type c)
	{
		return static_cast<_tString&&>(white::erase_right(str.find_last_of(c),
			str));
	}
	//! \since build 1.4
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_right(_tString&& str, const remove_reference_t<_tString>& t)
	{
		return static_cast<_tString&&>(white::erase_right(str.find_last_of(t),
			str));
	}
	/*!
	\pre ���ԣ�ָ������ǿա�
	\since build 1.4
	*/
	template<class _tString>
	inline wimpl(enable_if_t) < is_class<decay_t<_tString>>::value, _tString&& >
		erase_right(_tString&& str, typename string_traits<_tString>::const_pointer t
			= &to_array<typename string_traits<_tString>::value_type>(" \f\n\r\t\v")[0])
	{
		wconstraint(t);
		return static_cast<_tString&&>(white::erase_right(str.find_last_of(t),
			str));
	}
	//@}
	//@}

	//! \since build 1.3
	//@{
	//! \brief ɾ���ַ�����ָ��������ǰ׺�ַ���
	//@{
	template<class _tString>
	wconstfn _tString&&
		ltrim(_tString&& str, typename string_traits<_tString>::value_type c)
	{
		return static_cast<_tString&&>(
			details::str_algo<>::erase_left(str, str.find_first_not_of(c)));
	}
	//! \since build 1.4
	template<class _tString>
	inline _tString&&
		ltrim(_tString&& str, const _tString& t)
	{
		return static_cast<_tString&&>(
			details::str_algo<>::erase_left(str, str.find_first_not_of(t)));
	}
	/*!
	\pre ���ԣ�ָ������ǿա�
	\since build 1.4
	*/
	template<class _tString>
	wconstfn _tString&&
		ltrim(_tString&& str, typename string_traits<_tString>::const_pointer t
			= &to_array<typename string_traits<_tString>::value_type>(" \f\n\r\t\v")[0])
	{
		return wconstraint(t), static_cast<_tString&&>(
			details::str_algo<>::erase_left(str, str.find_first_not_of(t)));
	}
	//@}

	//! \brief ɾ���ַ�����ָ����������׺�ַ���
	//@{
	template<class _tString>
	wconstfn _tString&&
		rtrim(_tString&& str, typename string_traits<_tString>::value_type c)
	{
		return static_cast<_tString&&>(
			details::str_algo<>::erase_right(str, str.find_last_not_of(c)));
	}
	//! \since build 1.4
	template<class _tString>
	wconstfn _tString&&
		rtrim(_tString&& str, const remove_reference_t<_tString>& t) wnothrowv
	{
		return static_cast<_tString&&>(
			details::str_algo<>::erase_right(str, str.find_last_not_of(t)));
	}
	/*!
	\pre ���ԣ�ָ������ǿա�
	\since build 1.4
	*/
	template<class _tString>
	wconstfn _tString&&
		rtrim(_tString&& str, typename string_traits<_tString>::const_pointer t
			= &to_array<typename string_traits<_tString>::value_type>(" \f\n\r\t\v")[0])
	{
		return wconstraint(t), static_cast<_tString&&>(
			details::str_algo<>::erase_right(str, str.find_last_not_of(t)));
	}
	//@}

	//! \brief ɾ���ַ�����ָ��������ǰ׺���׺�ַ���
	//@{
	template<class _tString>
	wconstfn _tString&&
		trim(_tString&& str, typename string_traits<_tString>::value_type c)
	{
		return wforward(white::ltrim(wforward(white::rtrim(wforward(str), c)), c));
	}
	//! \since build 1.4
	template<class _tString>
	wconstfn _tString&&
		trim(_tString&& str, const _tString& t) wnothrowv
	{
		return wforward(white::ltrim(wforward(white::rtrim(wforward(str), t)), t));
	}
	/*!
	\pre ���ԣ�ָ������ǿա�
	\since build 1.4
	*/
	template<class _tString>
	wconstfn _tString&&
		trim(_tString&& str, typename string_traits<_tString>::const_pointer t
			= &to_array<typename string_traits<_tString>::value_type>(" \f\n\r\t\v")[0])
	{
		return wconstraint(t),
			wforward(white::ltrim(wforward(white::rtrim(wforward(str), t)), t));
	}
	//@}
	//@}
	//@}


	//! \since build 1.4
	//@{
	//! \brief ȡǰ׺���ַ�����
	//@{
	template<typename _tString, typename... _tParams>
	inline _tString
		find_prefix(const _tString& str, _tParams&&... args)
	{
		if (!str.empty())
		{
			const auto pos(str.find(wforward(args)...));

			if (pos != _tString::npos)
				return str.substr(0, pos);
		}
		return{};
	}
	//@}

	//! \brief ȡ��׺���ַ�����
	//@{
	template<typename _tString, typename... _tParams>
	inline _tString
		find_suffix(const _tString& str, _tParams&&... args)
	{
		if (!str.empty())
		{
			const auto pos(str.rfind(wforward(args)...));

			if (pos != _tString::npos)
				return str.substr(pos + 1);
		}
		return{};
	}
	//@}
	//@}

	/*!
	\brief ȡ��ͬ��ǰ׺�ͺ�׺Ԫ�ء�
	\pre ȡǰ׺�ͺ�׺�����޸����á�
	\pre �ַ����ǿա�
	\since build 1.4
	\todo ���ǳ�Ա \c front �� back \c ��
	\todo ֧��ǰ׺�ͺ�׺�ַ�����
	\todo ��չ��һ��������
	*/
	//@{
	template<typename _tString>
	wconstfn typename string_traits<_tString>::value_type
		get_quote_mark_nonstrict(const _tString& str)
	{
		return str.front() == str.back() ? str.front()
			: typename string_traits<_tString>::value_type();
	}
	//@}

	/*!
	\brief ȡ���ǰ׺�ͺ�׺���ַ�����
	\pre ���ԣ�ɾ�����ַ��������ڴ�����
	\since build 1.4
	*/
	//@{
	template<typename _tString>
	inline _tString
		quote(const _tString& str, typename string_traits<_tString>::value_type c
			= typename string_traits<_tString>::value_type('"'))
	{
		return c + str + c;
	}
	template<typename _tString, typename _tString2>
	inline wimpl(enable_if_t)<!is_convertible<_tString2,
		typename string_traits<_tString>::value_type>::value, _tString>
		quote(const _tString& str, const _tString2& s)
	{
		return s + str + s;
	}
	template<typename _tString, typename _tPrefix, typename _tSuffix>
	inline _tString
		quote(const _tString& str, const _tPrefix& pfx, const _tSuffix& sfx)
	{
		return pfx + str + sfx;
	}
	//@}

	/*!
	\brief ȡɾ��ǰ׺�ͺ�׺�����ַ�����
	\pre ���ԣ�ɾ�����ַ��������ڴ�����
	*/
	//@{
	template<typename _tString>
	inline _tString
		get_mid(const _tString& str, typename _tString::size_type l = 1)
	{
		wassume(!(str.size() < l * 2));
		return str.substr(l, str.size() - l * 2);
	}
	template<typename _tString>
	inline _tString
		get_mid(const _tString& str, typename _tString::size_type l,
			typename _tString::size_type r)
	{
		wassume(!(str.size() < l + r));
		return str.substr(l, str.size() - l - r);
	}
	//@}
	//@}


	/*!
	\brief ����������ȡ�ַ�����
	\since build 1.4
	*/
	//@{
	//! \note ͬ std::getline �����жϷָ����������ĸ������ɲ����ĺ������������
	template<typename _tChar, class _tTraits, class _tAlloc, typename _func>
	std::basic_istream<_tChar, _tTraits>&
		extract(std::basic_istream<_tChar, _tTraits>& is,
			std::basic_string<_tChar, _tTraits, _tAlloc>& str, _func f)
	{
		std::string::size_type n(0);
		auto st(std::ios_base::goodbit);

		if (const auto k
			= typename std::basic_istream<_tChar, _tTraits>::sentry(is, true))
		{
			const auto msize(str.max_size());
			const auto p_buf(is.rdbuf());

			wassume(p_buf);
			str.erase();
			try
			{
				const auto eof(_tTraits::eof());

				for (auto c(p_buf->sgetc()); ; c = p_buf->snextc())
				{
					if (_tTraits::eq_int_type(c, eof))
					{
						st =st | std::ios_base::eofbit;
						break;
					}
					if (f(c, *p_buf))
					{
						++n;
						break;
					}
					if (!(str.length() < msize))
					{
						st =st | std::ios_base::failbit;
						break;
					}
					str.append(1, _tTraits::to_char_type(c));
					++n;
				}
			}
			catch (...)
			{
				// NOTE: See http://wg21.cmeerw.net/lwg/issue91.
				rethrow_badstate(is, std::ios_base::badbit);
			}
		}
		if (n == 0)
			st =st | std::ios_base::failbit;
		if (st)
			is.setstate(st);
		return is;
	}

	//! \note ͬ std::getline �����ַ�����β�����ָ�����
	//@{
	template<typename _tChar, class _tTraits, class _tAlloc>
	std::basic_istream<_tChar, _tTraits>&
		extract_line(std::basic_istream<_tChar, _tTraits>& is,
			std::basic_string<_tChar, _tTraits, _tAlloc>& str, _tChar delim)
	{
		const auto d(_tTraits::to_int_type(delim));

		return white::extract(is, str,
			[d, &str](typename std::basic_istream<_tChar, _tTraits>::int_type c,
				std::basic_streambuf<_tChar, _tTraits>& sb) -> bool {
			if (_tTraits::eq_int_type(c, d))
			{
				sb.sbumpc();
				// NOTE: If not appended here, this function template shall be
				//	equivalent with %std::getline.
				str.append(1, d);
				return true;
			}
			return{};
		});
	}
	template<typename _tChar, class _tTraits, class _tAlloc>
	inline std::basic_istream<_tChar, _tTraits>&
		extract_line(std::basic_istream<_tChar, _tTraits>& is,
			std::basic_string<_tChar, _tTraits, _tAlloc>& str)
	{
		return white::extract_line(is, str, is.widen('\n'));
	}

	/*!
	\brief ͬ \c white::extract_line ��������ָ����������ӵ�ǰ׺�ַ���
	\note Ĭ�� \c LF ��Ϊ��׼�ָ�����ǰ׺Ϊ \c CR �������� \c LF �� <tt>CR+LF</tt> ��
	\note һ����϶����Ʒ�ʽ�򿪵���ʹ�ã��Ա��ⲻ��Ҫ�ķָ���ת����
	*/
	template<typename _tChar, class _tTraits, class _tAlloc>
	std::basic_istream<_tChar, _tTraits>&
		extract_line_cr(std::basic_istream<_tChar, _tTraits>& is,
			std::basic_string<_tChar, _tTraits, _tAlloc>& str, _tChar delim_cr = '\r',
			_tChar delim = '\n')
	{
		const auto cr(_tTraits::to_int_type(delim_cr));
		const auto d(_tTraits::to_int_type(delim));

		return white::extract(is, str,
			[cr, d, &str](typename std::basic_istream<_tChar, _tTraits>::int_type c,
				std::basic_streambuf<_tChar, _tTraits>& sb) -> bool {
			if (_tTraits::eq_int_type(c, d))
				str.append(1, d);
			else if (_tTraits::eq_int_type(c, cr)
				&& _tTraits::eq_int_type(sb.sgetc(), d))
			{
				sb.sbumpc();
				str.append(1, cr);
				str.append(1, d);
			}
			else
				return{};
			sb.sbumpc();
			return true;
		});
	}
	//@}
	//@}

	/*!
	\brief �Ǹ�ʽ�����
	\since build 1.4
	*/
	//@{
	template<typename _tChar, class _tString>
	std::basic_ostream<_tChar, typename _tString::traits_type>&
		write(std::basic_ostream<_tChar, typename _tString::traits_type>& os,
			const _tString& str, typename _tString::size_type pos = 0,
			typename _tString::size_type n = _tString::npos)
	{
		const auto len(str.length());

		if (pos < len)
			os.write(&str[pos], std::streamsize(std::min(n, len - pos)));
		return os;
	}
	//! \since build 1.4
	template<typename _tChar, class _tTraits, size_t _vN>
	std::basic_ostream<_tChar, _tTraits>&
		write(std::basic_ostream<_tChar, _tTraits>& os, const _tChar(&s)[_vN])
	{
		return os.write(std::addressof(s[0]), std::streamsize(size(s)));
	}

	/*!
	\note ����������Ϊ�ַ�����������
	\since build 1.4
	*/
	template<typename _tChar, class _tTraits, size_t _vN>
	std::basic_ostream<_tChar, _tTraits>&
		write_literal(std::basic_ostream<_tChar, _tTraits>& os, const _tChar(&s)[_vN])
	{
		static_assert(0 < _vN, "Empty string literal found.");

		return
			os.write(std::addressof(s[0]), std::streamsize(size(s) - 1));
	}
	//@}

	/*!
	\brief ת��Ϊ�ַ����� std::basic_string ��ʵ������
	\note �����׼���ͬ�����������Ա���ĳЩ����ת�����棬�� G++ �� [-Wsign-promo] ��
	*/
	//@{
	//! \since build 1.3
	//@{
	inline std::string
		to_string(unsigned char val)
	{
		return std::to_string(unsigned(val));
	}	
	inline std::string
		to_string(unsigned short val)
	{
		return std::to_string(unsigned(val));
	}
	template<typename _type>
	inline wimpl(enable_if_t)<is_enum<_type>::value,std::string>
		to_string(_type val)
	{
		using std::to_string;
		using white::to_string;

		return to_string(underlying(val));
	}
	template<typename _type, class _tStream = std::ostringstream>
	wimpl(enable_if_t)<is_class<_type>::value,
		decltype(std::declval<_tStream&>().str())>
		to_string(const _type& x)
	{
		_tStream oss;

		oss << x;
		return oss.str();
	}
	inline std::string
		to_string(const std::string& str)
	{
		return str;
	}
	//@}
	

	//! \since build 1.4
	//@{
	inline std::wstring
		to_wstring(unsigned char val)
	{
		return std::to_wstring(unsigned(val));
	}
	inline std::wstring
		to_wstring(unsigned short val)
	{
		return std::to_wstring(unsigned(val));
	}
	//! \since build 1.4
	template<typename _type>
	inline wimpl(enable_if_t)<is_enum<_type>::value, std::wstring>
		to_wstring(_type val)
	{
		using std::to_wstring;
		using white::to_wstring;

		return to_wstring(white::underlying(val));
	}
	template<typename _type, class _tStream = std::wostringstream>
	wimpl(enable_if_t)<is_class<_type>::value,
		decltype(std::declval<_tStream&>().str())>
		to_wstring(const _type& x)
	{
		_tStream oss;

		oss << x;
		return oss.str();
	}
	inline std::wstring
		to_wstring(const std::wstring& str)
	{
		return str;
	}
	//@}
	//@}

	namespace details
	{

		template<typename _tString, typename _type>
		struct ston_dispatcher;

#define WB_Impl_String_ston_begin(_tString, _type, _n, ...) \
	template<> \
	struct ston_dispatcher<_tString, _type> \
	{ \
		static _type \
		cast(const _tString& str, __VA_ARGS__) \
		{ \
			return _n(str
#define WB_Impl_String_ston_end \
			); \
		} \
	};

#define WB_Impl_String_ston_i(_tString, _type, _n) \
	WB_Impl_String_ston_begin(_tString, _type, _n, size_t* idx = {}, \
		int base = 10), idx, base \
	WB_Impl_String_ston_end
#define WB_Impl_String_ston_i_std(_type, _n) \
	WB_Impl_String_ston_i(std::string, _type, std::_n)
		WB_Impl_String_ston_i_std(int, stoi)
			WB_Impl_String_ston_i_std(long, stol)
			WB_Impl_String_ston_i_std(unsigned long, stoul)
			WB_Impl_String_ston_i_std(long long, stoll)
			WB_Impl_String_ston_i_std(unsigned long long, stoull)
#undef WB_Impl_String_ston_i_std
#undef WB_Impl_String_ston_i

#define WB_Impl_String_ston_f(_tString, _type, _n) \
	WB_Impl_String_ston_begin(_tString, _type, _n, size_t* idx = {}), idx \
	WB_Impl_String_ston_end
#define WB_Impl_String_ston_f_std(_type, _n) \
	WB_Impl_String_ston_f(std::string, _type, std::_n)
#	ifndef __BIONIC__
			WB_Impl_String_ston_f_std(float, stof)
			WB_Impl_String_ston_f_std(double, stod)
			WB_Impl_String_ston_f_std(long double, stold)
#	endif
#undef WB_Impl_String_ston_f_std
#undef WB_Impl_String_ston_f

#undef WB_Impl_String_ston_end
#undef WB_Impl_String_ston_begin

	} // namespace details;


	/*!
	\brief ת����ʾ�����ַ�����
	\since build 1.4
	\todo ֧�� std::string ���������ַ�����
	\todo ֧�ֱ�׼�������ת����
	*/
	template<typename _type, typename _tString, typename... _tParams>
	inline _type
		ston(const _tString& str, _tParams&&... args)
	{
		return details::ston_dispatcher<decay_t<_tString>, _type>::cast(str,
			wforward(args)...);
	}



	/*!
	\ingroup string_algorithms
	\brief ����ǰ׺������ǰ׺ʱ�����Ƴ�ǰ׺����Ӵ���
	\since build 1.4
	*/
	template<typename _type, typename _tString, typename _func>
	bool
		filter_prefix(const _tString& str, const _type& prefix, _func f)
	{
		if (white::begins_with(str, prefix))
		{
			auto&& sub(str.substr(string_length(prefix)));

			if (!sub.empty())
				f(std::move(sub));
			return true;
		}
		return{};
	}

	/*!
	\brief ѡ���Թ���ǰ׺��
	\return ָ��ǰ׺����ʱΪȥ��ǰ׺�Ĳ��֣�����Ϊ����ָ�������ֵ��
	\since build 1.4
	*/
	template<typename _type, typename _tString>
	_tString
		cond_prefix(const _tString& str, const _type& prefix, _tString&& val = {})
	{
		white::filter_prefix(str, prefix, [&](_tString&& s)
			wnoexcept(is_nothrow_move_assignable<decay_t<_tString>>::value) {
			val = std::move(s);
		});
		return std::move(val);
	}

	template<typename _tString,typename _tPattern,typename _tReplace>
	std::size_t replace_all(_tString& out, const _tPattern& pattern, const _tReplace& replace)
	{
		auto patternLength = string_length(pattern);
		auto replaceLength = string_length(replace);

		if (patternLength == 0)
			return 0;

		typename _tString::size_type pos = 0;
		std::size_t count = 0;

		while ((pos = out.find(pattern, pos)) != _tString::npos)
		{
			out.replace(pos, patternLength, replace);

			pos += replaceLength;

			++count;
		}

		return count;
	}

	
}
#endif