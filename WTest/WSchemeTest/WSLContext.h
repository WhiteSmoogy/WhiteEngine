/*!	\file WSLContext.h
\ingroup WTest
\brief WSL 上下文。
\par 修改时间:
	2018-05-26 14:14 +0800
*/


#ifndef WTest_WScheme_WSLContext_h_
#define WTest_WScheme_WSLContext_h_ 1

#include <WScheme/SContext.h>
#include <WScheme/WScheme.h>
#include <WScheme/WSchemREPL.h>

namespace scheme
{

namespace v1
{

LiteralPasses::HandlerType
	FetchExtendedLiteralPass();

} // namespace v1;

} // namespace scheme;

#endif

