/*!	\file WException.h
\ingroup WFrameWork/Core
\brief 异常处理模块。
*/

#ifndef WFrameWork_Core_WException_h
#define WFrameWork_Core_WException_h 1

#include <WBase/functional.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <WFramework/Core/WShellDefinition.h>
#include <WFramework/Adaptor/WTextBase.h>

namespace white
{
	//! \ingroup exceptions
	//@{
	//! \brief 一般运行时异常事件类。
	using GeneralEvent = std::runtime_error;


	//! \brief 记录日志的异常事件类。
	class WB_API LoggedEvent : public GeneralEvent
	{
	private:
		RecordLevel level;

	public:
		/*!
		\brief 构造：使用异常字符串和记录等级。
		*/
		//@{
		//! \pre 间接断言：第一参数非空。
		WB_NONNULL(2)
			LoggedEvent(const char* = "", RecordLevel = Emergent);
		LoggedEvent(const string_view, RecordLevel = Emergent);
		//@}
		/*!
		\brief 构造：使用一般异常事件对象和记录等级。
		*/
		LoggedEvent(const GeneralEvent&, RecordLevel = Emergent);
		DefDeCopyCtor(LoggedEvent)
			/*!
			\brief 虚析构：类定义外默认实现。
			*/
		virtual	~LoggedEvent() override;

		RecordLevel GetLevel() const wnothrow;
	};


	/*!
	\brief 致命错误。
	\note 库不直接捕获这个类和派生类的异常。
	*/
	class WB_API FatalError : public GeneralEvent
	{
	private:
		/*!
		\invariant \c content 。
		\todo 使用引用计数实现的字符串。
		*/
		shared_ptr<string> content;

	public:
		/*!
		\brief 构造：使用标题和内容。
		\pre 间接断言：第一参数和第二参数的数据指针非空。
		\note 复制字符串参数。
		*/
		WB_NONNULL(2)
			FatalError(const char*, string_view);
		DefDeCopyCtor(FatalError)
			/*!
			\brief 虚析构：类定义外默认实现。
			*/
			~FatalError() override;

		string_view GetContent() const wnothrow;
		const char* GetTitle() const wnothrow;
	};
	//@}


	//@{
	/*!
	\brief 打印带有层次信息的函数类型。
	\note 约定第一个参数非空。
	*/
	using ExtractedLevelPrinter
		= std::function<void(const char*, RecordLevel, size_t)>;
	template<typename _type>
	using GLevelTracer = std::function<void(_type, RecordLevel)>;
	using ExceptionTracer = GLevelTracer<const std::exception&>;


	/*!
	\brief 通过 WCL_TraceRaw 跟踪带空格缩进层次的异常信息的函数类型。
	\pre 断言：第一参数非空。
	*/
	WB_API WB_NONNULL(1) void
		TraceException(const char*, RecordLevel = Err, size_t level = 0) wnothrow;

	/*!
	\brief 通过 WCL_TraceRaw 跟踪记录异常类型。
	\todo 处理类型名称。
	*/
	WB_API void
		TraceExceptionType(const std::exception&, RecordLevel = Err) wnothrow;

	/*!
	\brief 使用 TraceException 展开和跟踪异常类型和信息。
	\sa ExtraceException
	\sa TraceException
	\sa TraceExceptionType
	*/
	WB_API void
		ExtractAndTrace(const std::exception&, RecordLevel = Err) wnothrow;

	//! \brief 展开指定层次的异常并使用指定参数记录。
	WB_API void
		ExtractException(const ExtractedLevelPrinter&,
			const std::exception&, RecordLevel = Err, size_t = 0) wnothrow;
	//@}

	//! \return 是否发生并捕获异常。
	//@{
	/*!
	\brief 执行并试图记录异常。

	对参数指定的函数求值，并使用最后一个参数跟踪记录异常。
	*/
	WB_API bool
		TryExecute(std::function<void()>, const char* = {}, RecordLevel = Alert,
			ExceptionTracer = ExtractAndTrace);

	/*!
	\brief 调用函数并试图返回。
	*/
	template<typename _fCallable, typename... _tParams>
	nonvoid_result_t<invoke_result_t<_fCallable ,_tParams...>>
		TryInvoke(_fCallable&& f, _tParams&&... args) wnothrow
	{
		TryRet(white::invoke_nonvoid(wforward(f), wforward(args)...))
			CatchExpr(std::exception& e, TraceExceptionType(e, Emergent))
			CatchExpr(..., WF_TraceRaw(Emergent, "Unknown exception found."))
			return{};
	}

	/*!
	\brief 调用函数并过滤宿主异常。
	\note 使用 ADL \c TryInvoke 和 \c TryExecute 。

	对参数指定的函数求值，并捕获和跟踪记录所有异常。
	*/
	template<typename _func>
	bool
		FilterExceptions(_func f, const char* desc = {}, RecordLevel lv = Alert,
			ExceptionTracer trace = ExtractAndTrace) wnothrow
	{
		return !TryInvoke([=] {
			return !TryExecute(f, desc, lv, trace);
		});
	}
	//@}

} // namespace white;



#endif
