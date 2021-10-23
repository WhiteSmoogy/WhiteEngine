/*!	\file path.hpp
\ingroup WBase
\brief ·��ģ�塣
*/

#ifndef FrameWork__LPath_hpp
#define FrameWork__LPath_hpp 1

#include "id.hpp"
#include "string.hpp"
#include "operators.hpp"
#include <limits>
#include <stdexcept>
#include <string_view>

namespace white {

	//@{
	//! \note ������·�������ػ���
	//@{
	//! \brief ·��������
	template<typename _type>
	class path_traits
	{
		using value_type = _type;
	};

	//! \brief �ļ��ַ���·��������
	template<typename _tChar, class _tAlloc>
	class path_traits<std::basic_string<_tChar, _tAlloc>>
	{
	public:
		using char_type = _tChar;
		using value_type = std::basic_string<_tChar, _tAlloc>;
		using view_type = std::basic_string_view<char_type>;

		static wconstfn bool
			is_parent(view_type sv) wnothrow
		{
			return sv.length() == 2 && sv[0] == '.' && sv[1] == '.';
		}

		static wconstfn bool
			is_root(view_type sv) wnothrow
		{
			return sv.empty();
		}

		static wconstfn bool
			is_self(const value_type& str) wnothrow
		{
			return str.length() == 1 && str[0] == '.';
		}
	};
	//@}


	/*!
	\brief ·�����
	\since build 543
	*/
	enum class path_category : wimpl(size_t)
	{
		empty,
			self,
			parent,
			node
	};

	/*!
	\brief ·�����ࡣ
	\relates path_category
	*/
	template<class _tString, class _tTraits = path_traits<_tString>>
	path_category
		classify_path(const _tString& name) wnothrow
	{
		if WB_UNLIKELY(name.empty())
			return path_category::empty;
		if (_tTraits::is_self(name))
			return path_category::self;
		if (_tTraits::is_parent(name))
			return path_category::parent;
		return path_category::node;
	}


	/*!
	\brief һ��·��ģ�塣
	\tparam _tSeqCon �ɵ��õ������������͡�
	\pre _tSeqCon ������������Ҫ��� LessThanComparable Ҫ��
	\pre _tSeqCon ��Ա�쳣��ȫ�Է���һ������Ҫ�󣬾��� ISO C++11
	�� 101 ָ���Ŀ�ѡ���� back �� front �� pop_back �� push_back ��
	\note ���������������ɵ�������Ҫ��֧�������Ƚϲ�����
	\note ʹ�� ISO C++11 ����Ҫ��ָ���ĳ�Ա˳��������
	\warning ����������
	\see ISO C++11 17.6.3.1 [utility.arg.requirements],
	23.2.1 [container.requirements.general] ��
	*/
	template<class _tSeqCon,
		class _tTraits = path_traits<typename _tSeqCon::value_type>>
		class lpath : private sequence_container_adaptor<_tSeqCon>,
		private dividable<lpath<_tSeqCon, _tTraits>, typename _tSeqCon::value_type>,
		private totally_ordered<lpath<_tSeqCon, _tTraits>>,
		private dividable<lpath<_tSeqCon, _tTraits>>
	{
	private:
		//@{
		using base = sequence_container_adaptor<_tSeqCon>;

	public:
		using value_type = typename _tSeqCon::value_type;
		using traits_type = _tTraits;
		using reference = typename _tSeqCon::reference;
		using const_reference = typename _tSeqCon::const_reference;
		using size_type = typename base::size_type;
		using difference_type = typename base::difference_type;
		using iterator = typename base::iterator;
		using const_iterator = typename base::const_iterator;
		using reverse_iterator = typename base::container_type::reverse_iterator;
		using const_reverse_iterator
			= typename base::container_type::const_reverse_iterator;

		lpath() = default;
		using base::base;
		lpath(const lpath&) = default;
		lpath(lpath&&) = default;

		lpath&
			operator=(const lpath& pth)
		{
			return *this = lpath(pth);
		}
		lpath&
			operator=(lpath&& pth) wnothrow
		{
			pth.swap(*this);
			return *this;
		}
		lpath&
			operator=(std::initializer_list<value_type> il)
		{
			return *this = lpath(il);
		}

		lpath&
			operator/=(const value_type& s)
		{
			if (!empty() && traits_type::is_parent(s))
			{
				if (!traits_type::is_root(front()) || 1U < size())
				{
					if (!(traits_type::is_parent(back())
						|| traits_type::is_self(back())))
						pop_back();
					else
						push_back(s);
				}
			}
			else if (!traits_type::is_self(s))
				push_back(s);
			return *this;
		}
		lpath&
			operator/=(const lpath& pth)
		{
			if (pth.is_relative())
				for (const auto& s : pth)
					*this /= s;
			return *this;
		}

		//@{
		friend bool
			operator==(const lpath& x, const lpath& y)
		{
			return static_cast<const base&>(x) == static_cast<const base&>(y);
		}

		friend bool
			operator<(const lpath& x, const lpath& y)
		{
			return static_cast<const base&>(x) < static_cast<const base&>(y);
		}
		//@}

		using base::begin;

		using base::end;

		using base::cbegin;

		using base::cend;

		using base::container_type::rbegin;

		using base::container_type::rend;

		using base::container_type::crbegin;

		using base::container_type::crend;

		using base::size;

		using base::max_size;

		using base::empty;

		using base::back;

		//	using base::emplace;

		//	using base::emplace_back;

		using base::front;

		using base::pop_back;

		using base::push_back;

		using base::insert;

		using base::erase;

		using base::clear;

		using base::assign;

		using base::get_container;

		void
			filter_self() wnothrow
		{
			// NOTE: Lambda is needed to support possibly overloaded function.
			erase_all_if(*this, [](const value_type& s) wnothrow{
				return traits_type::is_self(s);
			});
		}

		value_type
			get_root() const
		{
			return is_absolute() ? front() : value_type();
		}

		bool
			is_absolute() const wnothrow
		{
			return !empty() && traits_type::is_root(front());
		}

		bool
			is_relative() const wnothrow
		{
			return !is_absolute();
		}

		void
			merge_parents()
		{
			for (auto i(this->begin()); i != this->end(); )
			{
				auto j(std::adjacent_find(i, this->end(),
					[&](const value_type& x, const value_type& y) {
					return !traits_type::is_self(x) && !traits_type::is_parent(
						x) && traits_type::is_parent(y);
				}));

				if (j == this->end())
					break;
				i = j++;
				wassume(j != this->end());
				i = erase(i, ++j);
			}
		}

		void
			swap(lpath& pth) wnothrow
		{
			base::swap(static_cast<base&>(pth));
		}
		//@}
	};
	//@}

	/*!
	\relates path
	*/
	//@{
	//! \brief ���滯��
	template<class _tSeqCon, class _tTraits>
	inline void
		normalize(lpath<_tSeqCon, _tTraits>& pth)
	{
		pth.filter_self(), pth.merge_parents();
	}

	//! \brief ������
	template<class _tSeqCon, class _tTraits>
	inline void
		swap(lpath<_tSeqCon, _tTraits>& x, lpath<_tSeqCon, _tTraits>& y)
	{
		x.swap(y);
	}

	//! \brief ȡ�ַ�����ʾ��
	template<class _tSeqCon, class _tTraits>
	typename _tSeqCon::value_type
		to_string(const lpath<_tSeqCon, _tTraits>& pth,
			const typename _tSeqCon::value_type& seperator = &to_array<
			typename string_traits<typename _tSeqCon::value_type>::value_type>("/")[0])
	{
		static_assert(is_object<typename _tSeqCon::value_type>(),
			"Invalid type found.");

		if (pth.empty())
			return{};

		auto i(pth.begin());
		typename _tSeqCon::value_type res(*i);

		while (++i != pth.end())
		{
			res += seperator;
			res += *i;
		}
		return res;
	}

	//! \brief ȡ�Էָ����������ַ�����ʾ��
	template<class _tSeqCon, class _tTraits>
	typename _tSeqCon::value_type
		to_string_d(const lpath<_tSeqCon, _tTraits>& pth, typename string_traits<typename
			_tSeqCon::value_type>::value_type delimiter = '/')
	{
		static_assert(is_object<typename _tSeqCon::value_type>(),
			"Invalid type found.");
		typename _tSeqCon::value_type res;

		for (const auto& s : pth)
		{
			res += s;
			res += delimiter;
		}
		return res;
	}
	//@}
}


#endif
