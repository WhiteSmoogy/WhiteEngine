/*! \file typeinfo.h
\ingroup WBase
\brief ISO C++ ������Ϣ��չ��

*/
#ifndef WBase_typeinfo_h
#define WBase_typeinfo_h 1

#include "sutility.h" //for noncopyable;
#include "type_traits.hpp" // for is_void;
#include <typeinfo> //for std::type_info,typeid;
#include <functional>

namespace white {
	//! \since build 1.4
	//@{
	/*!
	\brief ������Ϣ��
	\note ���� std::type_info ������ʹ�� \c name ʱ�ɱ�������������š�
	\warning ����������
	*/
	class WB_API type_id_info : private wimpl(noncopyable)
	{
	public:
		using id_t = size_t;

	private:
		id_t id;

		wimpl(wconstfn)
			type_id_info(id_t i = 0)
			: id(i)
		{}

	public:
		wimpl(wconstfn) bool
			operator==(const type_id_info& rhs) const wnothrow
		{
			return id == rhs.id;
		}

		wimpl(wconstfn) bool
			operator!=(const type_id_info& rhs) const wnothrow
		{
			return id != rhs.id;
		}

		wimpl(wconstfn) bool
			before(const type_id_info& rhs) const wnothrow
		{
			return id < rhs.id;
		}

		id_t
			get() const wnothrow
		{
			return id;
		}

		size_t
			hash_code() const wnothrow
		{
			return std::hash<id_t>()(id);
		}

		template<typename _type>
		static const type_id_info&
			make()
		{
			return is_void<_type>() ? xref_void() : xref<remove_cv_t<_type>>();
		}

		wimpl(wconstfn) const char*
			name() const wnothrow
		{
			return "<unknown>";
		}

	private:
		static id_t
			push()
		{
			static id_t id;

			return ++id;
		}

		template<typename _type>
		static const type_id_info&
			xref()
		{
			static_assert(is_same<_type, remove_cv_t<_type>>(),
				"Invalid type found.");
			static type_id_info info(push());

			return info;
		}

		static const type_id_info&
			xref_void()
		{
			static type_id_info info;

			return info;
		}
	};


	/*!
	\def WB_Use_LightweightTypeID
	\brief ʹ���������ͱ�ʶ���� typeid ��
	\warning ������ RTTI ����ģ��ʹ����Ϊ����ʵ�֣�����֤Ψһ�ԡ�
	\since build 1.4
	*/
#if WB_Use_LightweightTypeID
	using type_info = type_id_info;
#else
	using std::type_info;
#endif


	//! \sa type_info
	template<typename _type>
	const type_info&
		type_id()
	{
#if WB_Use_LightweightTypeID
		return type_info::make<_type>();
#else
		return typeid(_type);
#endif
	}
	//@}
}



#endif