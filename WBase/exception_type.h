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
	//! \brief 异常：不支持的操作。
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
		\brief 虚析构：类定义外默认实现。
		\since build 1.4
		*/
		~unsupported() override;
	};


	//! \brief 异常：未实现的操作。
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
		\brief 虚析构：类定义外默认实现。
		\since build 1.4
		*/
		~unimplemented() override;
	};
	//@}

	/*!
	\brief 异常：无法表示数值/枚举范围内的转换。
	*/
	class WB_API narrowing_error : public std::logic_error
	{
	public:
		narrowing_error()
			: logic_error("Norrowing found.")
		{}
		using logic_error::logic_error;
		narrowing_error(const narrowing_error&) = default;
		//! \brief 虚析构：类定义外默认实现。
		~narrowing_error() override;
	};
	//@}


	class WB_API invalid_construction : public std::invalid_argument
	{
	public:
		invalid_construction();
		invalid_construction(const invalid_construction&) = default;

		/*!
		\brief 虚析构：类定义外默认实现。
		*/
		~invalid_construction() override;
	};
}
