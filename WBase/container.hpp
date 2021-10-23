/*!	\file container.hpp
\ingroup WBase
\brief ͨ������������
*/

#ifndef WBase_container_hpp
#define WBase_container_hpp 1

#include "iterator.hpp" // for begin, end, make_transform, size, std::distance,
//	std::make_move_iterator, cbegin, cend, std::piecewise_construct_t,
//	std::piecewise_construct;
#include "functional.hpp" // for std::declval, is_detected_convertible;
#include "utility.hpp" // for std::initializer_list, white::as_const;
#include "algorithm.hpp" // for is_undereferenceable, sort_unique;

namespace white
{
	/*!
	\brief ������������
	\note ��������Ҫ�󣨵����� ISO C++ Ҫ�����������Ҫ�󣩡�
	\note ʹ�� ISO C++11 ����Ҫ��ָ���ĳ�Ա˳��������
	\warning ����������
	\see ISO C++11 23.6 [container.adaptors],
	23.2.1 [container.requirements.general] ��
	\since build 1.4
	*/
	template<class _tSeqCon>
	class container_adaptor : protected _tSeqCon
	{
	protected:
		using container_type = _tSeqCon;

	private:
		using base = container_type;

	public:
		//! \brief ��������Ҫ��
		//@{
		using value_type = typename container_type::value_type;
		using reference = typename container_type::reference;
		using const_reference = typename container_type::const_reference;
		using iterator = typename container_type::iterator;
		using const_iterator = typename container_type::const_iterator;
		using difference_type = typename container_type::difference_type;
		using size_type = typename container_type::size_type;

		container_adaptor() = default;
		explicit
			container_adaptor(size_type n)
			: base(n)
		{}
		container_adaptor(size_type n, const value_type& value)
			: base(n, value)
		{}
		template<class _tIn>
		container_adaptor(_tIn first, _tIn last)
			: base(std::move(first), std::move(last))
		{}
		container_adaptor(const container_adaptor&) = default;
		container_adaptor(container_adaptor&&) = default;
		//@}
		container_adaptor(std::initializer_list<value_type> il)
			: base(il)
		{}

		//! \brief ��������Ҫ��
		//@{
		container_adaptor&
			operator=(const container_adaptor&) = default;
		container_adaptor&
			operator=(container_adaptor&&) = default;
		//@}
		container_adaptor&
			operator=(std::initializer_list<value_type> il)
		{
			base::operator=(il);
		}

		//! \brief ��������Ҫ��
		friend bool
			operator==(const container_adaptor& x, const container_adaptor& y)
		{
			return static_cast<const container_type&>(x)
				== static_cast<const container_type&>(y);
		}

		//! \brief ��������Ҫ��
		//@{
		using container_type::begin;

		using container_type::end;

		using container_type::cbegin;

		using container_type::cend;

		//! \since build 1.4
		container_type&
			get_container() wnothrow
		{
			return *this;
		}

		//! \since build 1.4
		const container_type&
			get_container() const wnothrow
		{
			return *this;
		}

		void
			swap(container_adaptor& con) wnothrow
		{
			return base::swap(static_cast<container_type&>(con));
		}

		using base::size;

		using base::max_size;

		using base::empty;
		//@}
	};

	/*!
	\brief ��������Ҫ��
	\since build 1.4
	*/
	//@{
	template<class _tSeqCon>
	inline bool
		operator!=(const container_adaptor<_tSeqCon>& x,
			const container_adaptor<_tSeqCon>& y)
	{
		return !(x == y);
	}

	template<class _tSeqCon>
	void
		swap(container_adaptor<_tSeqCon>& x,
			container_adaptor<_tSeqCon>& y) wnothrow
	{
		x.swap(y);
	}
	//@}


	/*!
	\brief ����������������
	\note ��������Ҫ�󣨵����� ISO C++ Ҫ�����������Ҫ�󣩡�
	\note ʹ�� ISO C++11 ����Ҫ��ָ���ĳ�Ա˳��������
	\warning ����������
	\see ISO C++11 23.6 [container.adaptors],
	23.2.1 [container.requirements.general], 23.2.3 [sequence.reqmts] ��
	\since build 408
	*/
	template<class _tSeqCon>
	class sequence_container_adaptor : protected container_adaptor<_tSeqCon>
	{
	private:
		using base = container_adaptor<_tSeqCon>;

	public:
		using container_type = typename base::container_type;
		using value_type = typename container_type::value_type;
		using size_type = typename container_type::size_type;

		//! \brief ������������Ҫ��
		//@{
		sequence_container_adaptor() = default;
		explicit
			sequence_container_adaptor(size_type n)
			: base(n)
		{}
		sequence_container_adaptor(size_type n, const value_type& value)
			: base(n, value)
		{}
		template<class _tIn>
		sequence_container_adaptor(_tIn first, _tIn last)
			: base(std::move(first), std::move(last))
		{}
		sequence_container_adaptor(const sequence_container_adaptor&) = default;
		sequence_container_adaptor(sequence_container_adaptor&&) = default;
		sequence_container_adaptor(std::initializer_list<value_type> il)
			: base(il)
		{}

		sequence_container_adaptor&
			operator=(const sequence_container_adaptor&) = default;
		sequence_container_adaptor&
			operator=(sequence_container_adaptor&&) = default;
		sequence_container_adaptor&
			operator=(std::initializer_list<value_type> il)
		{
			base::operator=(il);
		}

		//! \brief ��������Ҫ��
		friend bool
			operator==(const sequence_container_adaptor& x,
				const sequence_container_adaptor& y)
		{
			return static_cast<const container_type&>(x)
				== static_cast<const container_type&>(y);
		}

		//	using container_type::emplace;

		using container_type::insert;

		using container_type::erase;

		using container_type::clear;

		using container_type::assign;
		//@}

		//! \since build 1.4
		using base::get_container;
	};

	/*!
	\brief ��������Ҫ��
	\since build 1.4
	*/
	//@{
	template<class _tSeqCon>
	inline bool
		operator!=(const sequence_container_adaptor<_tSeqCon>& x,
			const sequence_container_adaptor<_tSeqCon>& y)
	{
		return !(x == y);
	}

	template<class _tSeqCon>
	void
		swap(sequence_container_adaptor<_tSeqCon>& x,
			sequence_container_adaptor<_tSeqCon>& y) wnothrow
	{
		x.swap(y);
	}
	//@}


	/*!
	\ingroup helper_functions
	\brief ����ָ�����͵�������
	\since build 1.3
	*/
	//@{
	template<class _tCon, typename _tIter>
	inline _tCon
		make_container(_tIter first, _tIter last)
	{
		return _tCon(first, last);
	}
	//! \note ʹ�� ADL \c begin �� \c end ָ����Χ��������
	//@{
	template<class _tCon, typename _tRange>
	inline _tCon
		make_container(_tRange&& c)
	{
		return _tCon(begin(wforward(c)), end(wforward(c)));
	}
	template<class _tCon, typename _tRange, typename _func>
	inline _tCon
		make_container(_tRange&& c, _func f)
	{
		return _tCon(white::make_transform(begin(c), f),
			white::make_transform(end(c), f));
	}
	//@}
	template<class _tCon, typename _tElem>
	inline _tCon
		make_container(std::initializer_list<_tElem> il)
	{
		return _tCon(il.begin(), il.end());
	}
	//@}


	//! \since build 1.3
	namespace details
	{

		//! \since build 1.4
		//@{
		template<class _type>
		using range_size_t = decltype(size(std::declval<_type>()));

		template<class _type>
		using has_range_size = is_detected_convertible<size_t, range_size_t, _type>;
		//@}

		template<typename _tRange>
		wconstfn auto
			range_size(const _tRange& c, true_type) -> decltype(size(c))
		{
			return size(c);
		}
		template<typename _tRange>
		inline auto
			range_size(const _tRange& c, false_type)
			-> decltype(std::distance(begin(c), end(c)))
		{
			return std::distance(begin(c), end(c));
		}

	} // namespace details;

	  /*!
	  \ingroup algorithms
	  \pre ����ָ���ĵ�������Χ�������ڣ���Ч��
	  \note ���� \c first �� \c last ָ����������Χ��
	  \note �Բ��Ե�����ָ���ķ�Χ��ʹ�� ADL begin �� end ȡ��������ֵ�������ͣ�
	  ��ȷ��Ϊ const ������ʱʹ�� ADL cbegin �� cend ���档
	  */
	  //@{
	  /*!
	  \brief ȡ��Χ��С��
	  \since build 1.3

	  �� std::initializer_list ��ʵ��ֱ�ӷ��ش�С������
	  ���ɵ��ý����ת��Ϊ \c size_t �� ADL ���� \c size ��ʹ�� ADL \c size ��
	  ����ʹ�� std::distance ���㷶Χ������ȷ����Χ��С��
	  */
	  //@{
	template<typename _tRange>
	wconstfn auto
		range_size(const _tRange& c)
		-> decltype(details::range_size(c, details::has_range_size<_tRange>()))
	{
		return details::range_size(c, details::has_range_size<_tRange>());
	}
#if __cplusplus <= 201402L
	/*!
	\see http://wg21.cmeerw.net/cwg/issue1591 ��
	\since build 1.4
	*/
	template<typename _tElem>
	wconstfn size_t
		range_size(std::initializer_list<_tElem> il)
	{
		return size(il);
	}
#endif
	//@}


	/*!
	\brief �������ָ����Ԫ�ص�������
	\since build 1.3
	*/
	//@{
	template<class _tCon, typename... _tParams>
	inline void
		assign(_tCon& con, _tParams&&... args)
	{
		con.assign(wforward(args)...);
	}
	template<class _tCon, typename _type, size_t _vN>
	inline void
		assign(_tCon& con, const _type(&arr)[_vN])
	{
		con.assign(arr, arr + _vN);
	}
	//@}


	/*!
	\brief ����Ԫ�ص�����ĩβ��
	\pre ָ��Ԫ�صķ�Χ���������ص���
	\note ʹ�� ADL end ��
	\since build 1.3
	\todo ���ط� \c void ��
	*/
	//@{
	template<class _tCon, typename _tIn>
	void
		concat(_tCon& con, _tIn first, _tIn last)
	{
		con.insert(end(con), std::make_move_iterator(first),
			std::make_move_iterator(last));
	}
	template<class _tCon, typename _tRange>
	void
		concat(_tCon& con, _tRange&& c)
	{
		con.insert(end(con), begin(wforward(c)), end(wforward(c)));
	}
	//@}


	/*!
	\brief ������뷶Χ��������ָ��λ�á�
	\param con ָ����������
	\param i ָ����ʼ����λ�õĵ�������
	\param first ���뷶Χ��ʼ��������
	\param last ���뷶Χ��ֹ��������
	\pre i �� con �ĵ�������
	\since build 1.3

	���ô��е����������� \c insert ��Ա�������������ָ���ķ�Χ��������ָ��λ�á�
	con �ǹ�������ʱ����Χ��Ԫ������ɱ��Ż���
	*/
	template<typename _tCon, typename _tIn>
	typename _tCon::const_iterator
		insert_reversed(_tCon& con, typename _tCon::const_iterator i, _tIn first,
			_tIn last)
	{
		for (; first != last; ++first)
		{
			wconstraint(!is_undereferenceable(first));
			i = con.insert(i, *first);
		}
		return i;
	}
	//@}


	/*!
	\brief �������뺯������
	\note ��Ա�������� ISO C++11 24.5.2 �е��ඨ���Ҫ��
	\since build 1.3
	*/
	template<typename _tCon>
	class container_inserter
	{
	public:
		using container_type = _tCon;

	protected:
		_tCon* container;

	public:
		container_inserter(_tCon& con)
			: container(&con)
		{}

		template<typename... _tParams>
		auto
			operator()(_tParams&&... args)
			-> decltype(container->insert(wforward(args)...))
		{
			wassume(container);
			return container->insert(wforward(args)...);
		}
	};

	/*!
	\brief ˳�����ֵ��ָ��������
	\since build 1.3
	*/
	template<typename _tCon, typename... _tParams>
	inline void
		seq_insert(_tCon& con, _tParams&&... args)
	{
		white::seq_apply(container_inserter<_tCon>(con), wforward(args)...);
	}


	//! \since build 1.3
	namespace details
	{

		template<class _tCon, typename _tKey>
		bool
			exists(const _tCon& con, const _tKey& key,
				decltype(std::declval<_tCon>().count()) = 1U)
		{
			return con.count(key) != 0;
		}
		template<class _tCon, typename _tKey>
		bool
			exists(const _tCon& con, const _tKey& key, ...)
		{
			return con.find(key) != end(con);
		}


		//! \since build 1.4
		//@{
		template<class _type, typename... _tParams>
		using mem_remove_t
			= decltype(std::declval<_type>().remove(std::declval<_tParams>()...));

		template<typename _type, typename... _tParams>
		using has_mem_remove = is_detected<mem_remove_t, _type, _tParams...>;

		template<class _type, typename... _tParams>
		using mem_remove_if_t
			= decltype(std::declval<_type>().remove_if(std::declval<_tParams>()...));

		template<typename _type, typename... _tParams>
		using has_mem_remove_if = is_detected<mem_remove_if_t, _type, _tParams...>;

		template<class _type>
		using key_type_t = typename _type::key_type;

		template<typename _type>
		using has_mem_key_type = is_detected<key_type_t, _type>;


		template<typename _tSeqCon, typename _type>
		inline void
			erase_remove(_tSeqCon& con, decltype(begin(con)) first,
				decltype(end(con)) last, const _type& value)
		{
			con.erase(std::remove(first, last, value), last);
		}

		template<typename _tSeqCon, typename _fPred>
		inline void
			erase_remove_if(_tSeqCon& con, decltype(begin(con)) first,
				decltype(end(con)) last, _fPred pred)
		{
			con.erase(std::remove_if(first, last, pred), last);
		}

		template<typename _tSeqCon, typename _type>
		inline void
			erase_all_in_seq(_tSeqCon& con, _type&& value, true_type)
		{
			con.remove(wforward(value));
		}
		template<typename _tSeqCon, typename _type>
		inline void
			erase_all_in_seq(_tSeqCon& con, const _type& value, false_type)
		{
			details::erase_remove(con, begin(con), end(con), value);
		}

		template<typename _tSeqCon, typename _fPred,typename = void>
		struct erase_all_if_in_seq
		{
			void operator()(_tSeqCon& con, _fPred pred) {
				details::erase_remove_if(con, begin(con), end(con), pred);
			}
		};

		template<typename _tSeqCon, typename _fPred>
		struct erase_all_if_in_seq<_tSeqCon,_fPred,std::void_t<decltype(std::declval<_tSeqCon>().remove_if(std::declval<_fPred>()))>>
		{
			void operator()(_tSeqCon& con, _fPred pred) {
				con.remove_if(pred);
			}
		};

		template<typename _tCon, typename _type>
		void
			erase_all(_tCon& con, decltype(cbegin(con)) first, decltype(cend(con)) last,
				const _type& value, true_type)
		{
			while (first != last)
				if (*first == value)
					con.erase(first++);
				else
					++first;
		}
		template<typename _tCon, typename _type>
		inline void
			erase_all(_tCon& con, const _type& value, true_type)
		{
			details::erase_all(con, cbegin(con), cend(con), value, true_type());
		}
		template<typename _tCon, typename _type>
		inline void
			erase_all(_tCon& con, decltype(begin(con)) first, decltype(end(con)) last,
				const _type& value, false_type)
		{
			details::erase_remove(con, first, last, value);
		}
		template<typename _tCon, typename _type>
		inline void
			erase_all(_tCon& con, const _type& value, false_type)
		{
			details::erase_all_in_seq(con, value, has_mem_remove<_tCon>());
		}

		template<typename _tCon, typename _fPred>
		void
			erase_all_if(_tCon& con, decltype(cbegin(con)) first, decltype(cend(con)) last,
				_fPred pred, true_type)
		{
			while (first != last)
				if (pred(*first))
					con.erase(first++);
				else
					++first;
		}
		template<typename _tCon, typename _fPred>
		inline void
			erase_all_if(_tCon& con, _fPred pred, true_type)
		{
			details::erase_all_if(con, cbegin(con), cend(con), pred, true_type());
		}
		template<typename _tCon, typename _fPred>
		inline void
			erase_all_if(_tCon& con, decltype(begin(con)) first, decltype(end(con)) last,
				_fPred pred, false_type)
		{
			details::erase_remove_if(con, first, last, pred);
		}
		template<typename _tCon, typename _fPred>
		inline void
			erase_all_if(_tCon& con, _fPred pred, false_type)
		{
			details::erase_all_if_in_seq<_tCon,_fPred>()(con, pred);
		}
		//@}

	} // namespace details;

	  //! \ingroup algorithms
	  //@{
	  /*!
	  \brief �ж�ָ���������д���ָ���ļ���
	  \note ������������ֵ��ʹ�÷�����������ʼ�������͵ĳ�Ա \c count ʱʹ��
	  ��Ա \c count ʵ�֣�����ʹ�� ADL \c end ָ����������������
	  ʹ�ó�Ա \c find ʵ�֡�
	  \since build 1.3
	  */
	template<class _tCon, typename _tKey>
	inline bool
		exists(const _tCon& con, const _tKey& key)
	{
		return details::exists(con, key);
	}


	/*!
	\note �Բ��� key_type ��������Ϊ����������
	\since build 1.4
	*/
	//@{
	/*!
	\brief ɾ��ָ��������ָ�������еĻ�ȫ��Ԫ�ء�
	\note ��������������ʹ�� white::remove �Ƴ�Ԫ�ط�Χ��
	\note �����������еĲ�������ʹ�� std::remove �Ƴ�Ԫ�ط�Χ��
	*/
	//@{
	template<typename _tCon, typename _tIter>
	inline void
		erase_all(_tCon& con, _tIter first, _tIter last)
	{
		con.erase(first, last);
	}
	template<typename _tCon, typename _tIter, typename _type>
	inline void
		erase_all(_tCon& con, _tIter first, _tIter last, const _type& value)
	{
		details::erase_all(con, first, last, value,
			details::has_mem_key_type<_tCon>());
	}
	template<typename _tCon, typename _type>
	inline void
		erase_all(_tCon& con, const _type& value)
	{
		details::erase_all(con, value, details::has_mem_key_type<_tCon>());
	}
	//@}

	/*!
	\brief ɾ��ָ��������ָ�������еĻ�ȫ������ν�ʵ�Ԫ�ء�
	\note ��������������ʹ�� white::remove_if �Ƴ�Ԫ�ط�Χ��
	\note �����������еĲ�������ʹ�� std::remove_if �Ƴ�Ԫ�ط�Χ��
	*/
	//@{
	template<typename _tCon, typename _tIter, typename _fPred>
	inline void
		erase_all_if(_tCon& con, _tIter first, _tIter last, _fPred pred)
	{
		details::erase_all_if(con, first, last, pred,
			details::has_mem_key_type<_tCon>());
	}
	template<typename _tCon, typename _fPred>
	inline void
		erase_all_if(_tCon& con, _fPred pred)
	{
		details::erase_all_if(con, pred, details::has_mem_key_type<_tCon>());
	}
	//@}
	//@}

	//! \since build 1.4
	//@{
	/*!
	\brief ɾ��ָ�����������еȼ���ָ��������ʼԪ�ء�
	\return �Ƿ�ɹ�ɾ����
	*/
	template<class _tAssocCon, typename _tKey>
	bool
		erase_first(_tAssocCon& con, const _tKey& k)
	{
		const auto i(con.find(k));

		if (i != end(con))
		{
			con.erase(i);
			return true;
		}
		return{};
	}

	/*!
	\brief ɾ��ָ�����������еȼ���ָ����������Ԫ�ء�
	\return ɾ����Ԫ������
	*/
	//@{
	template<class _tAssocCon>
	inline typename _tAssocCon::size_type
		erase_multi(_tAssocCon& con, const typename _tAssocCon::key_type& k)
	{
		return con.erase(k);
	}
	template<class _tAssocCon, typename _tKey>
	typename _tAssocCon::size_type
		erase_multi(_tAssocCon& con, const _tKey& k)
	{
		const auto pr(con.equal_range(k));

		if (pr.first != pr.second)
		{
			const auto n(std::distance(pr.first, pr.second));

			con.erase(pr.first, pr.second);
			return n;
		}
		return 0;
	}
	//@}
	//@}

	/*!
	\brief ɾ��ָ��������ָ����������ʼָ��������Ԫ�ء�
	\pre ָ���ĵ�������ָ�������ĵ�������
	\pre ���Լ�飺ɾ���ķ�Χ������������
	\since build 1.3
	*/
	//@{
	template<typename _tCon>
	typename _tCon::const_iterator
		erase_n(_tCon& con, typename _tCon::const_iterator i,
			typename _tCon::difference_type n)
	{
		wassume(n <= std::distance(i, cend(con)));
		return con.erase(i, std::next(i, n));
	}
	//! \since build 1.3
	template<typename _tCon>
	typename _tCon::iterator
		erase_n(_tCon& con, typename _tCon::iterator i,
			typename _tCon::difference_type n)
	{
		wassume(n <= std::distance(i, end(con)));
		return con.erase(i, std::next(i, n));
	}
	//@}


	/*!
	\brief �Ƴ� const_iterator �� const �޶���
	\since build 1.4
	*/
	//@{
	template<class _tCon>
	inline typename _tCon::iterator
		cast_mutable(_tCon& con, typename _tCon::const_iterator i)
	{
		return con.erase(i, i);
	}
	template<class _tCon, typename _type>
	inline std::pair<typename _tCon::iterator, _type>
		cast_mutable(_tCon& con,
			const std::pair<typename _tCon::const_iterator, _type>& pr)
	{
		return{ white::cast_mutable(con, pr.first), pr.second };
	}
	//@}


	/*!
	\brief ����ָ�������������������ظ�Ԫ�ء�
	\pre �����ĵ������������������Ҫ��
	\since build 1.3
	*/
	template<class _tCon>
	inline void
		sort_unique(_tCon& con)
	{
		con.erase(white::sort_unique(begin(con), end(con)), end(con));
	}


	/*!
	\brief ��֤�ǿ������Բ���ָ����Ԫ�ؽ�β��
	\since build 1.4
	*/
	//@{
	template<class _tCon>
	_tCon&&
		trail(_tCon&& con, const typename decay_t<_tCon>::value_type& value = {})
	{
		if (!con.empty() && !(con.back() == value))
			con.push_back(value);
		return static_cast<_tCon&&>(con);
	}

	namespace details {
		//TODO 
		template<size_t idx, typename... _tParams>
		struct params_element
		{
			using tuple = std::tuple<_tParams...>;
			using type = std::conditional_t<(idx >=sizeof...(_tParams)), void, std::tuple_element_t<idx, tuple>>;
		};
	}

	template<class _tCon, typename... _tParams,
		typename = enable_if_t<(sizeof...(_tParams) > 1) ||
		sizeof...(_tParams) == 1 &&
			!std::is_same_v<
				remove_cv_t<details::params_element<0,_tParams...>>,
				typename decay_t<_tCon>::value_type
	>>>
	_tCon&&
		trail(_tCon&& con, _tParams&&... args)
	{
		if (!con.empty())
		{
			typename decay_t<_tCon>::value_type val(wforward(args)...);

			if (!(con.back() == val))
				con.emplace_back(std::move(val));
		}
		return static_cast<_tCon&&>(con);
	}
	//@}


	/*!
	\brief ������ĩβ����ָ��ֵ��Ԫ�����Ƴ���
	\since build 1.4
	*/
	template<class _tCon>
	bool
		pop_back_val(_tCon& con, const typename _tCon::value_type& value)
	{
		if (!con.empty() && con.back() == value)
		{
			con.pop_back();
			return true;
		}
		return{};
	}

	/*!
	\brief ��������ʼ����ָ��ֵ��Ԫ�����Ƴ���
	\since build 1.4
	*/
	template<class _tCon>
	bool
		pop_front_val(_tCon& con, const typename _tCon::value_type& value)
	{
		if (!con.empty() && con.front() == value)
		{
			con.pop_front();
			return true;
		}
		return{};
	}


	/*!
	\brief ����Ԫ�ص� \c vector ĩβ��
	\note ʹ�� ADL \c size ��
	\since build 1.4
	*/
	//@{
	template<class _tVector, typename _tIn>
	void
		vector_concat(_tVector& vec, _tIn first, _tIn last)
	{
		vec.reserve(size(vec) + std::distance(first, last));
		white::concat(vec, first, last);
	}
	template<class _tVector, typename _tRange>
	void
		vector_concat(_tVector& vec, _tRange&& c)
	{
		vec.reserve(size(vec) + white::range_size(c));
		white::concat(vec, wforward(c));
	}
	//@}


	/*!
	\brief �ӵ�һ����ָ������ʼ��������Сѭ��ȡ����������������󻺳�����
	\pre ���ԣ���һ���������� 0 ��
	\note ������֧�� \c max_size �� \c resize ���ַ������͡�
	\see http://wg21.cmeerw.net/lwg/issue2466 ��
	\since build 1.4
	*/
	template<class _tVector, typename _func>
	_tVector
		retry_for_vector(typename _tVector::size_type s, _func f)
	{
		wconstraint(s != 0);

		_tVector res;

		for (res.resize(s); f(res, s) && s < res.max_size() wimpl(/ 2); )
			res.resize(s *= wimpl(2));
		return res;
	}



	/*!
	\ingroup helper_functions
	\brief �滻����������ֵ��
	\note ʹ�� \c end ָ����Χ��������
	\since build 1.3
	\todo ֧��û�� \c emplace_hint ��Ա�Ĺ���������
	*/
	template<class _tAssocCon, typename _tKey, typename _func>
	auto
		replace_value(_tAssocCon& con, const _tKey& k, _func f)
		-> decltype(con.emplace_hint(con.erase(con.find(k)), f(*con.find(k))))
	{
		auto i(con.find(k));

		return i != end(con) ? con.emplace_hint(con.erase(i), f(*i)) : i;
	}


	/*!
	\ingroup metafunctions
	\brief �жϹ������������Ͳ��������Ƿ���԰� map �ķ�ʽ��ת�Ƶع��졣
	\since build 1.4
	*/
	template<class _tAssocCon, typename _tKey, typename... _tParams>
	using is_piecewise_mapped = is_constructible<typename _tAssocCon::value_type,
		std::piecewise_construct_t, std::tuple<_tKey&&>, std::tuple<_tParams&&...>>;


	namespace details
	{
		template<class _tAssocCon>
		struct assoc_con_traits
		{
			//@{
			template<typename _tKey, typename... _tParams>
			static inline typename _tAssocCon::iterator
				emplace_hint_in_place(false_, _tAssocCon& con,
					typename _tAssocCon::const_iterator hint, _tKey&&, _tParams&&... args)
			{
				return con.emplace_hint(hint, wforward(args)...);
			}
			template<typename _tKey, typename... _tParams>
			static inline typename _tAssocCon::iterator
				emplace_hint_in_place(true_, _tAssocCon& con,
					typename _tAssocCon::const_iterator hint, _tKey&& k, _tParams&&... args)
			{
				return con.emplace_hint(hint, std::piecewise_construct,
					std::forward_as_tuple(wforward(k)),
					std::forward_as_tuple(wforward(args)...));
			}

			static inline const typename _tAssocCon::key_type&
				extract_key(false_, const typename _tAssocCon::value_type& val)
			{
				return val;
			}
			static inline const typename _tAssocCon::key_type&
				extract_key(true_, const typename _tAssocCon::value_type& val)
			{
				return val.first;
			}
			//@}
		};

		template<class _type>
		using mapped_type_t = typename _type::mapped_type;


		template<class _tAssocCon, typename _tKey, typename _tParam>
		std::pair<typename _tAssocCon::iterator, bool>
			insert_or_assign(std::pair<typename _tAssocCon::iterator, bool> pr,
				_tAssocCon& con, _tKey&& k, _tParam&& arg)
		{
			if (pr.first)
				pr.first
				= emplace_hint_in_place(con, pr.second, wforward(k), wforward(arg));
			else
				pr.second = wforward(arg);
			return pr;
		}

	} // unnamed namespace details;

	/*!
	\brief ������ʾ��ԭ�ز��빹�졣
	\since build 1.4
	*/
	//@{
	/*!
	\brief ������ʾ��ԭ�ز��빹�졣
	*/
	template<class _tAssocCon, typename _tKey, typename... _tParams>
	inline auto
		emplace_hint_in_place(_tAssocCon& con, typename _tAssocCon::const_iterator hint,
			_tKey&& k, _tParams&&... args) -> typename _tAssocCon::iterator
	{
		return details::assoc_con_traits<_tAssocCon>::emplace_hint_in_place(
			is_piecewise_mapped<_tAssocCon, _tKey, _tParams...>(), con, hint,
			wforward(k), wforward(args)...);
	}
	//@}

	//! \since build 1.4
	namespace details
	{

		template<class _type>
		using mapped_type_t = typename _type::mapped_type;

		template<class _tAssocCon>
		inline const typename _tAssocCon::key_type&
			extract_key(const typename _tAssocCon::value_type& val, false_type)
		{
			return val;
		}
		template<class _tAssocCon>
		inline const typename _tAssocCon::key_type&
			extract_key(const typename _tAssocCon::value_type& val, true_type)
		{
			return val.first;
		}


		

	} // unnamed namespace details;

	  /*!
	  \brief �ӹ���������ֵȡ����
	  \since build 1.4
	  */
	  //@{
	template<class _tAssocCon, typename _type,
		wimpl(typename = enable_if_convertible_t<const _type&,
			const typename _tAssocCon::value_type&>)>
		inline const typename _tAssocCon::key_type&
		extract_key(const _type& val)
	{
		return details::extract_key<_tAssocCon>(val,
			is_detected<details::mapped_type_t, _tAssocCon>());
	}
	template<class _tAssocCon, typename _tKey,
		wimpl(typename = enable_if_inconvertible_t<const _tKey&,
			const typename _tAssocCon::value_type&>)>
		inline const _tKey&
		extract_key(const _tKey& k)
	{
		return k;
	}
	//@}


	/*!
	\return һ�����ڱ�ʾ����� std::pair ֵ�����Ա \c first Ϊ��������
	\c second ��ʾ�Ƿ�û���ҵ���
	\note ʹ�� ADL cend �� extract_key ��
	*/
	//@{
	/*!
	\brief ��ָ��������ָ������������
	\since build 1.4
	*/
	//@{
	template<class _tAssocCon, typename _tKey>
	std::pair<typename _tAssocCon::const_iterator, bool>
		search_map(const _tAssocCon& con, const _tKey& k)
	{
		const auto i(con.lower_bound(k));

		return{ i, i == end(con) || con.key_comp()(k, extract_key<_tAssocCon>(*i)) };
	}
	template<class _tAssocCon, typename _tKey>
	inline std::pair<typename _tAssocCon::iterator, bool>
		search_map(_tAssocCon& con, const _tKey& k)
	{
		return white::cast_mutable(con, white::search_map(white::as_const(con), k));
	}
	//@}
	//! \since build 1.4
	//@{
	/*!
	\brief ��ָ��������ʾ�ĵ�����λ������ָ������������
	\pre �����ǿջ���ʾ�ĵ�����λ��ָ��β����
	\note ʹ�� ADL cbegin ��
	*/
	//@{
	template<class _tAssocCon, typename _tKey>
	std::pair<typename _tAssocCon::const_iterator, bool>
		search_map(const _tAssocCon& con, typename _tAssocCon::const_iterator hint,
			const _tKey& k)
	{
		if (!con.empty())
		{
			const auto& comp(con.key_comp());
			const bool fit_before(hint == cbegin(con)
				|| bool(comp(extract_key(*std::prev(hint)), k))),
				fit_after(hint == cend(con)
					|| bool(comp(k, extract_key(*std::next(hint)))));

			if (fit_before == fit_after)
				return{ hint, fit_before && fit_after };
			return white::search_map(con, k);
		}
		wconstraint(hint == cend(con));
		return{ hint, true };
	}
	template<class _tAssocCon, typename _tKey>
	inline std::pair<typename _tAssocCon::iterator, bool>
		search_map(_tAssocCon& con, typename _tAssocCon::const_iterator hint,
			const _tKey& k)
	{
		return white::cast_mutable(con,
			white::search_map(white::as_const(con), hint, k));
	}
	//@}
	/*!
	\brief ��ָ����ֵ����ָ��ӳ�䲢ִ�в�����
	\pre ���Ĳ��������µ�ֵ��
	\return �����Ա�ĵ�������
	\note ��Ϊ���� std::map::operator[] ��
	*/
	//@{
	template<typename _func, class _tAssocCon, typename... _tParams>
	std::pair<typename _tAssocCon::const_iterator, bool>
		search_map_by(_func f, const _tAssocCon& con, _tParams&&... args)
	{
		auto pr(white::search_map(con, wforward(args)...));

		if (pr.second)
			pr.first = f(pr.first);
		return pr;
	}
	template<typename _func, class _tAssocCon, typename... _tParams>
	inline std::pair<typename _tAssocCon::iterator, bool>
		search_map_by(_func f, _tAssocCon& con, _tParams&&... args)
	{
		return white::cast_mutable(con,
			white::search_map_by(f, white::as_const(con), wforward(args)...));
	}
	//@}
	//@}
	//@}

	/*!
	\note ʹ�� ADL emplace_hint_in_place ��
	\sa emplace_hint_in_place
	\see WG21 N4279 ��
	*/
	//@{
	//! \since build 1.4
	//@{
	template<class _tAssocCon, typename _tKey, typename... _tParams>
	std::pair<typename _tAssocCon::iterator, bool>
		try_emplace(_tAssocCon& con, _tKey&& k, _tParams&&... args)
	{
		// XXX: Blocked. 'wforward' may cause G++ 5.2 silent crash.
		return white::search_map_by([&](typename _tAssocCon::const_iterator i) {
			return emplace_hint_in_place(con, i, wforward(k),
				std::forward<_tParams>(args)...);
		}, con, k);
	}

	template<class _tAssocCon, typename _tKey, typename... _tParams>
	std::pair<typename _tAssocCon::iterator, bool>
		try_emplace_hint(_tAssocCon& con, typename _tAssocCon::const_iterator hint,
			_tKey&& k, _tParams&&... args)
	{
		// XXX: Blocked. 'wforward' may cause G++ 5.2 silent crash.
		return white::search_map_by([&](typename _tAssocCon::const_iterator i) {
			return emplace_hint_in_place(con, i, wforward(k),
				std::forward<_tParams>(args)...);
		}, con, hint, k);
	}
	//@}

	//! \since build 1.4
	//@{
	template<class _tAssocCon, typename _tKey, typename _tParam>
	inline std::pair<typename _tAssocCon::iterator, bool>
		insert_or_assign(_tAssocCon& con, _tKey&& k, _tParam&& arg)
	{
		return details::insert_or_assign(white::search_map(con, k), con,
			wforward(k), wforward(arg));
	}

	template<class _tAssocCon, typename _tKey, typename _tParam>
	inline std::pair<typename _tAssocCon::iterator, bool>
		insert_or_assign_hint(_tAssocCon& con, typename _tAssocCon::const_iterator hint,
			_tKey&& k, _tParam&& arg)
	{
		return details::insert_or_assign(white::search_map(con, hint, k), con,
			wforward(k), wforward(arg));
	}
	//@}
	//@}
	//@}

} // namespace white;



#endif