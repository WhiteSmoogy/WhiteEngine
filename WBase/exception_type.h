#pragma once

#include "wdef.h"
#include <stdexcept> // for std::logic_error;

namespace white
{
	/*!
	\since build 1.3
	\ingroup exceptions
	*/
	//@{
	//! \brief �쳣����֧�ֵĲ�����
	class WB_API unsupported : public std::logic_error
	{
	public:
		unsupported()
			: logic_error("Unsupported operation found.")
		{}
		//! \since build 1.4
		using logic_error::logic_error;
		//! \since build 1.4
		unsupported(const unsupported&) = default;
		/*!
		\brief ���������ඨ����Ĭ��ʵ�֡�
		\since build 1.4
		*/
		~unsupported() override;
	};


	//! \brief �쳣��δʵ�ֵĲ�����
	class WB_API unimplemented : public unsupported
	{
	public:
		unimplemented()
			: unsupported("Unimplemented operation found.")
		{}
		//! \since build 1.3
		using unsupported::unsupported;
		//! \since build 1.4
		unimplemented(const unimplemented&) = default;
		/*!
		\brief ���������ඨ����Ĭ��ʵ�֡�
		\since build 1.4
		*/
		~unimplemented() override;
	};
	//@}

	/*!
	\brief �쳣���޷���ʾ��ֵ/ö�ٷ�Χ�ڵ�ת����
	*/
	class WB_API narrowing_error : public std::logic_error
	{
	public:
		narrowing_error()
			: logic_error("Norrowing found.")
		{}
		using logic_error::logic_error;
		narrowing_error(const narrowing_error&) = default;
		//! \brief ���������ඨ����Ĭ��ʵ�֡�
		~narrowing_error() override;
	};
	//@}


	class WB_API invalid_construction : public std::invalid_argument
	{
	public:
		invalid_construction();
		invalid_construction(const invalid_construction&) = default;

		/*!
		\brief ���������ඨ����Ĭ��ʵ�֡�
		*/
		~invalid_construction() override;
	};
}
