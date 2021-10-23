/*! \file exception.h
\ingroup WBase
\brief ��׼���쳣��չ�ӿڡ�
*/

#ifndef WBase_exception_h
#define WBase_exception_h 1

#include "deref_op.hpp" // for call_value_or;
#include "type_traits.hpp" // for remove_cv_t;
#include <memory> // for std::addressof;
#include <functional> // for std::mem_fn;
#include <stdexcept> // for std::logic_error;
#include <system_error> // for std::error_category, std::generic_category,
//	std::system_error, std::errc;
#include "exception_type.h"

namespace white
{
	/*!	\defgroup exceptions Exceptions
	\brief �쳣���͡�
	\since build 1.4
	*/


	/*!
	\brief ���������쳣��
	\since build 1.4
	*/
	template<typename _func>
	void
		iterate_exceptions(_func&& f, std::exception_ptr p = std::current_exception())
	{
		while (p)
			p = wforward(f)(p);
	}

	/*!
	\brief ȡǶ���쳣ָ�롣
	\return ������ std::rethrow_if_nested �׳��쳣�������򷵻��쳣ָ�룬����Ϊ�ա�
	\since build 1.4
	*/
	template<class _tEx>
	inline std::exception_ptr
		get_nested_exception_ptr(const _tEx& e)
	{
		return white::call_value_or(std::mem_fn(&std::nested_exception::nested_ptr),
			dynamic_cast<const std::nested_exception*>(std::addressof(e)));
	}

	/*!
	\brief �����쳣������ǰ�������ڴ�����쳣���׳�Ƕ���쳣�������׳��쳣��
	\since build 1.4
	*/
	template<typename _type>
	WB_NORETURN inline void
		raise_exception(_type&& e)
	{
		if (std::current_exception())
			std::throw_with_nested(wforward(e));
		throw wforward(e);
	}


	/*!
	\brief ����Ƕ���쳣��
	\since build 1.4
	*/
	template<typename _tEx, typename _func,
		typename _tExCaught = remove_cv_t<_tEx>&>
		void
		handle_nested(_tEx& e, _func&& f)
	{
		try
		{
			std::rethrow_if_nested(e);
		}
		catch (_tExCaught ex)
		{
			wforward(f)(ex);
		}
	}

	
	/*!
	\brief �׳� invalid_construction �쳣��
	\throw invalid_construction
	\relates invalid_construction
	*/
	WB_NORETURN WB_API void
		throw_invalid_construction();

	WB_NORETURN WB_API void
		throw_out_of_range(const char* msg);

	/*!
	\brief �׳����� std::system_error ��������ͬ���캯���������쳣��
	\throw _type ʹ�ò���������쳣��
	\since build 1.4
	*/
	//@{
	template<class _type = std::system_error, typename... _tParams>
	WB_NORETURN inline void
		throw_error(std::error_code ec, _tParams&&... args)
	{
		throw _type(ec, wforward(args)...);
	}
	template<class _type = std::system_error, typename... _tParams>
	WB_NORETURN inline void
		throw_error(std::error_condition cond, _tParams&&... args)
	{
		throw _type(cond.value(), cond.category(), wforward(args)...);
	}
	template<class _type = std::system_error, typename... _tParams>
	WB_NORETURN inline void
		throw_error(int ev, _tParams&&... args)
	{
		throw _type(ev, std::generic_category(), wforward(args)...);
	}
	template<class _type = std::system_error, typename... _tParams>
	WB_NORETURN inline void
		throw_error(int ev, const std::error_category& ecat, _tParams&&... args)
	{
		throw _type(ev, ecat, wforward(args)...);
	}
	//@}

} // namespace white;
#endif