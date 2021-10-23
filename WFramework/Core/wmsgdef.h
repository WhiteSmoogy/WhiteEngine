/*!	\file wmsgdef.h
\ingroup WFrameWork/Core
\brief 标准 Shell 消息列表。
*/


#ifndef WFrameWork_Core_lmsgdef_h
#define WFrameWork_Core_lmsgdef_h 1

#include <WFramework/Core/WMessage.h>
#include <functional> // for std::function;

namespace white
{

namespace Messaging
{

using MessageID = enum MessageSpace
{
	Null = 0x0000,
	Set = 0x0003,

	Quit = 0x0012,
	//! \since build 454
	Bound = 0x0014,
	Task = 0x0016,

	Paint = 0x00AF,

	Input = 0x00FF
};


//@{
#define SM_Null			white::Messaging::Null
#define SM_Set			white::Messaging::Set

#define SM_Quit			white::Messaging::Quit
#define SM_Bound		white::Messaging::Bound
#define SM_Task			white::Messaging::Task

#define SM_Paint		white::Messaging::Paint
#define SM_Input		white::Messaging::Input
//@}


template<MessageID _vID>
struct SMessageMap
{};

#define DefMessageTarget(_id, _type) \
	template<> \
	struct SMessageMap<_id> \
	{ \
		using TargetType = _type; \
	};

//@{
DefMessageTarget(SM_Null, void)
DefMessageTarget(SM_Set, shared_ptr<Shell>)
DefMessageTarget(SM_Quit, int)
DefMessageTarget(SM_Bound, pair<weak_ptr<Shell> WPP_Comma Message>)
DefMessageTarget(SM_Task, std::function<void()>)
DefMessageTarget(SM_Input, void)
//@}


template<MessageID _vID>
inline const typename SMessageMap<_vID>::TargetType&
FetchTarget(const Message& msg)
{
	return msg.GetContent().GetObject<typename SMessageMap<_vID>::TargetType>();
}

} // namespace Messaging;

} // namespace white;

#endif

