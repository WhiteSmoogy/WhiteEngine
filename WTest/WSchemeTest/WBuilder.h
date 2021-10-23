/*!	\file WBuilder.h
\ingroup WTest
\brief WSL 解释实现。
\par 修改时间:
	2018-05-26 13:16 +0800
*/


#ifndef WTest_WScheme_WBuilder_h_
#define WTest_WScheme_WBuilder_h_ 1

#include "Interpreter.h"

namespace scheme
{

/// 674
using namespace white;

namespace v1
{

/// 674
void
RegisterLiteralSignal(ContextNode&, const string&, SSignal);

} // namespace v1;

} // namespace scheme;

#endif

