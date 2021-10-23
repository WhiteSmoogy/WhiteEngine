/*!	\file Interpreter.h
\ingroup WTest
\brief WScheme 解释器。
\par 修改时间:
	2018-5-26 13:03 +0800
*/


#ifndef WTEST_WScheme_Interpreter_h_
#define WTEST_WScheme_Interpreter_h_ 1

#include "WSLContext.h"
#include <WFramework/Win32/WCLib/Consoles.h>
#include <WFramework/Core/WConsole.h>
#include <iosfwd>
#include <functional>

namespace scheme
{

/// 592
using namespace white::Consoles;
/// 674
using white::Logger;
/// 740
using v1::REPLContext;

/// 304
enum class SSignal
{
	Exit,
	ClearScreen,
	About,
	Help,
	License
};


/// 673
void
LogTree(const ValueNode&, Logger::Level = white::Debug);

void LogTermValue(const TermNode&, Logger::Level = white::Debug);

/*!
\build 控制台默认颜色。
*/
wconstexpr Color DefaultColor(Gray), TitleColor(Cyan),
	InfoColor(White), ErrorColor(Red), PromptColor(DarkGreen),
	SignalColor(DarkRed), SideEffectColor(Yellow), ReducedColor(Magenta);


class Interpreter
{
private:
	platform_ex::Windows::WConsole wc;
	white::RecordLevel err_threshold;
	//white::unique_ptr<white::Environment> p_env;
	string line;
	REPLContext context;

public:
	Interpreter(white::Application&, std::function<void(REPLContext&)>);

	~Interpreter();

	void
	HandleSignal(SSignal);

	bool
	Process();

	std::istream&
	WaitForLine();
	/// 696
	std::istream&
	WaitForLine(std::istream&, std::ostream&);
};

} // namespace NPL;

#endif

