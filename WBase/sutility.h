/*! \file sutility.h
\ingroup WBase
\brief ����ʵ����ʩ��
\par �޸�ʱ��:
2017-01-01 23:46 +0800
*/
#ifndef WBase_sutility_h
#define WBase_sutility_h 1

#include "wdef.h"

namespace white
{
	/*!
	\brief ���ɸ��ƶ��󣺽�ֹ���������Ĭ��ԭ�͵ĸ��ƹ��캯���͸��Ƹ�ֵ��������
	\warning ����������
	\since change 1.4
	*/
	class noncopyable
	{
	protected:
		/*!
		\brief \c protected ���죺Ĭ��ʵ�֡�
		\note �����Ƕ�̬�ࡣ
		*/
		wconstfn noncopyable() = default;
		//! \brief \c protected ������Ĭ��ʵ�֡�
		~noncopyable() = default;

	public:
		//! \brief ��ֹ���ƹ��졣
		wconstfn noncopyable(const noncopyable&) = delete;

		/*!
		\brief ����ת�ƹ��졣
		\since build 1.4
		*/
		wconstfn
			noncopyable(noncopyable&&) = default;

		//! \brief ��ֹ���Ƹ�ֵ��
		noncopyable&
			operator=(const noncopyable&) = delete;

		/*!
		\brief ����ת�Ƹ�ֵ��
		\since build 1.4
		*/
		noncopyable&
			operator=(noncopyable&&) = default;
	};

	/*
	\brief ����ת�ƶ��󣺽�ֹ�̳д���Ķ������Ĭ��ԭ�͵�ת�ƹ��캯����ת�Ƹ�ֵ��������
	\warning ����������
	\since change 1.4
	*/
	class nonmovable
	{
	protected:
		/*!
		\brief \c protected ���죺Ĭ��ʵ�֡�
		\note �����Ƕ�̬�ࡣ
		*/
		wconstfn nonmovable() = default;
		//! \brief \c protected ������Ĭ��ʵ�֡�
		~nonmovable() = default;

	public:
		//! \since build 1.4
		//@{
		//! \brief �����ƹ��졣
		wconstfn nonmovable(const nonmovable&) = default;
		//! \brief ��ֹת�ƹ��졣
		wconstfn
			nonmovable(nonmovable&&) = delete;

		//! \brief �����Ƹ�ֵ��
		nonmovable&
			operator=(const nonmovable&) = default;
		//! \brief ��ֹת�Ƹ�ֵ��
		nonmovable&
			operator=(const nonmovable&&) = delete;
		//@}
	};


	/*!
	\brief �ɶ�̬���Ƶĳ�����ࡣ
	*/
	class WB_API cloneable
	{
	public:
		cloneable() = default;
		cloneable(const cloneable&) = default;
		//! \brief ���������ඨ����Ĭ��ʵ�֡�
		virtual
		~cloneable();

		virtual cloneable*
		clone() const = 0;
	};

	/*!
	\brief ��Ӳ����������������������á�
	\note ������ ::indirect_input_iterator ��ת���������ʡ�
	\todo �ṩ�ӿ������û�ָ��ʹ�� ::polymorphic_cast �ȼ��ת����
	*/
	template<typename _type>
	struct deref_self
	{
		_type&
			operator*() wnothrow
		{
			return *static_cast<_type*>(this);
		}
		const _type&
			operator*() const wnothrow
		{
			return *static_cast<const _type*>(this);
		}
		volatile _type&
			operator*() volatile wnothrow
		{
			return *static_cast<volatile _type*>(this);
		}
		const volatile _type&
			operator*() const volatile wnothrow
		{
			return *static_cast<const volatile _type*>(this);
		}
	};

	/*!
	\brief ������ʵ�塣
	\note ���ӵ�ģ�������֤��ͬ�����͡�
	\warning ���ܷ������������ҽ����������������
	\since build CPP17
	*/
	template<class _tBase, typename...>
	class derived_entity : public _tBase
	{
	public:
		using base = _tBase;

		derived_entity() = default;
		using base::base;
		derived_entity(const base& b) wnoexcept_spec(base(b))
			: base(b)
		{}
		derived_entity(base&& b) wnoexcept_spec(base(std::move(b)))
			: base(std::move(b))
		{}
		derived_entity(const derived_entity&) = default;
		derived_entity(derived_entity&&) = default;

		derived_entity&
			operator=(const derived_entity&) = default;
		derived_entity&
			operator=(derived_entity&&) = default;
	};
}


#endif