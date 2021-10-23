/*!	\file cache.hpp
\ingroup WBase
\brief ���ٻ�������ģ�塣
*/

#ifndef WBase_cache_hpp
#define WBase_cache_hpp 1

#include "deref_op.hpp" // for std::pair, is_undereferenceable;
#include "cassert.h" // for wassume;
#include "type_op.hpp" // for are_same;
#include <list> // for std::list;
#include <unordered_map> // for std::unordered_map;
#include <map> // for std::map;
#include <stdexcept> // for std::runtime_error;

namespace white
{
	//@{
	/*!
	\brief ʹ��˫������ʵ�ֵ����ʹ���б�
	\note ��֤�Ƴ���ʱ��˳��Ϊ��Զ�����Ƴ���
	*/
	template<typename _tKey, typename _tMapped,
	class _tAlloc = std::allocator<std::pair<const _tKey, _tMapped>>>
	class recent_used_list : private std::list<std::pair<const _tKey, _tMapped>>
	{
	public:
		using allocator_type = _tAlloc;
		using value_type = std::pair<const _tKey, _tMapped>;
		using list_type = std::list<value_type>;
		using size_type = typename list_type::size_type;
		using const_iterator = typename list_type::const_iterator;
		using iterator = typename list_type::iterator;

		using list_type::begin;

		using list_type::cbegin;

		using list_type::cend;

		using list_type::clear;

		template<typename... _tParams>
		iterator
			emplace(_tParams&&... args)
		{
			list_type::emplace_front(wforward(args)...);
			return begin();
		}

		using list_type::empty;

		using list_type::end;

		using list_type::front;

		iterator
			refresh(iterator i) wnothrowv
		{
			list_type::splice(begin(), *this, i);
			return i;
		}

		template<typename _tMap>
		void
			shrink(_tMap& m) wnothrowv
		{
			wassume(!empty());

			auto& val(list_type::back());

			m.erase(val.first);
			list_type::pop_back();
		}
		template<typename _tMap, typename _func>
		void
			shrink(_tMap& m, _func f)
		{
			wassume(!empty());

			auto& val(list_type::back());

			if (f)
				f(val);
			m.erase(val.first);
			list_type::pop_back();
		}
		template<typename _tMap, typename... _tParams>
		void
			shrink(_tMap& m, size_type max_use, _tParams&&... args)
		{
			while (max_use < size_type(m.size()))
			{
				wassume(!m.empty());

				shrink(m, wforward(args)...);
			}
		}

		void
			undo_emplace() wnothrow
		{
			wassume(!empty());

			list_type::pop_front();
		}
	};


	//! \brief ���ˢ�²��ԵĻ���������
	//@{
	template<typename _tKey, typename _tMapped, typename _tPred, typename _fHash = std::hash<_tKey>,
	class _tAlloc = std::allocator<std::pair<const _tKey, _tMapped>>,
	class _tList = recent_used_list<_tKey, _tMapped, _tAlloc >>
	struct used_list_cache_traits
	{
		using map_type = std::unordered_map<_tKey, _tMapped, _fHash,
			_tPred, _tAlloc>;
		using used_list_type = _tList;
		using used_cache_type = std::unordered_map<_tKey,
			typename _tList::iterator, _fHash, typename map_type::key_equal,
			typename std::allocator_traits<_tAlloc>::template rebind_alloc<std::pair<const _tKey, typename _tList::iterator>>
			>;
	};

	template<typename _tKey, typename _tMapped, typename _tPred, class _tAlloc, class _tList>
	struct used_list_cache_traits<_tKey, _tMapped, _tPred, void, _tAlloc, _tList>
	{
		using map_type = std::map<_tKey, _tMapped, _tPred, _tAlloc>;
		using used_list_type = _tList;
		using used_cache_type = std::map<_tKey, typename _tList::iterator,
			typename map_type::key_compare, typename _tAlloc::template
			rebind<std::pair<const _tKey, typename _tList::iterator>>::other>;
	};
	//@}


	/*!
	\brief �����ʹ�ò���ˢ�µĻ��档
	\note Ĭ�ϲ����滻�������ʹ�õ�����������
	\todo �����쳣��ȫ�ĸ��ƹ��캯���͸��Ƹ�ֵ��������
	\todo ֧������ˢ�²��ԡ�
	*/
	template<typename _tKey, typename _tMapped,typename _tPred = std::less<_tKey>, typename _fHash = void,
		typename _tTraits = used_list_cache_traits<_tKey, _tMapped, _tPred, _fHash >>
	class used_list_cache
	{
	public:

		using traits_type = _tTraits;
		using map_type = typename traits_type::map_type;
		using key_type = typename map_type::key_type;

		using value_type = typename map_type::value_type;
		using size_type = typename map_type::size_type;
		using used_list_type = typename traits_type::used_list_type;
		using used_cache_type = typename traits_type::used_cache_type;
		using const_iterator = typename used_list_type::const_iterator;
		using iterator = typename used_list_type::iterator;

		static_assert(are_same<size_type, typename used_list_type::size_type,
			typename used_cache_type::size_type>::value, "Invalid size found.");

	private:
		/*!
		\invariant <tt>std::count(used_list.begin(), used_list.end())
		== used_cache.size()</tt> ��
		*/
		//@{
		mutable used_list_type used_list;
		used_cache_type used_cache;
		//@}
		//! \brief ���ֿ���������һ������������������
		size_type max_use;

	public:
		std::function<void(value_type&)> flush{};

		explicit
			used_list_cache(size_type s = wimpl(15U))
			: used_list(), used_cache(), max_use(s)
		{}
		used_list_cache(used_list_cache&&) = default;

	private:
		void
			check_max_used() wnothrowv
		{
			used_list.shrink(used_cache, max_use, flush);
		}

	public:
		size_type
			get_max_use() const wnothrow
		{
			return max_use;
		}

		void
			set_max_use(size_type s) wnothrowv
		{
			max_use = s < 1 ? 1 : s;
			check_max_used();
		}

		//@{
		iterator
			begin()
		{
			return used_list.begin();
		}
		iterator
			begin() const
		{
			return used_list.cbegin();
		}
		//@}

		void
			clear() wnothrow
		{
			used_list.clear(),
				used_cache.clear();
		}

		template<typename... _tParams>
		std::pair<iterator, bool>
			emplace(const key_type& k, _tParams&&... args)
		{
			check_max_used();

			const auto i_cache(used_cache.find(k));

			if (i_cache == used_cache.end())
			{
				const auto i(used_list.emplace(k, wforward(args)...));

				try
				{
					used_cache.emplace(k, i);
				}
				catch (...)
				{
					used_list.undo_emplace();
					throw;
				}
				return{ i, true };
			}
			else
				return{ i_cache->second, false };
		}
		template<typename... _tParams>
		std::pair<iterator, bool>
			emplace(_tParams&&... args)
		{
			check_max_used();

			const auto i(used_list.emplace(wforward(args)...));

			try
			{
				const auto pr(used_cache.emplace(i->first, i));

				if (!pr.second)
				{
					used_list.undo_emplace();
					return  { pr.first->second, false };
				}
			}
			catch (...)
			{
				used_list.undo_emplace();
				throw;
			}
			return { i, true };
		}

		//@{
		iterator
			end()
		{
			return used_list.end();
		}
		iterator
			end() const
		{
			return used_list.cend();
		}
		//@}

		iterator
			find(const key_type& k)
		{
			const auto i(used_cache.find(k));

			return i != used_cache.end() ? used_list.refresh(i->second) : end();
		}
		const_iterator
			find(const key_type& k) const
		{
			const auto i(used_cache.find(k));

			return i != used_cache.end() ? used_list.refresh(i->second) : end();
		}

		size_type
			size() const wnothrow
		{
			return used_cache.size();
		}
	};
	//@}


	/*!
	\brief ��ָ���Ĺؼ��ֲ�����Ϊ������������������
	��û���ҵ�ʹ��ָ���Ŀɵ��ö���Ͳ�����ʼ�����ݡ�
	\tparam _tMap ӳ�����ͣ������� std::map_type �� std::unordered_map
	�� white::used_list_cache �ȡ�
	*/
	template<class _tMap, typename _tKey, typename _func, typename... _tParams>
	auto
		cache_lookup(_tMap& cache, const _tKey& key, _func init, _tParams&&... args)
		-> decltype((cache.begin()->second))
	{
		auto i(cache.find(key));

		if (i == cache.end())
		{
			const auto pr(cache.emplace(key, init(wforward(args)...)));

			if WB_UNLIKELY(!pr.second)
				throw std::runtime_error("Cache insertion failed.");
			i = pr.first;
		}
		return i->second;
	}

} // namespace white;

#endif
