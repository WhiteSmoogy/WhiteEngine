#include "DrawEvent.h"
#include <cstdarg>
#include <WFramework/Core/WString.h>

using namespace platform::Render;

platform::Render::DrawEventName::DrawEventName(const char* EventFormat, ...)
{
	va_list valist;
	va_start(valist, EventFormat);
	FormatedEventName = white::Text::String(white::vsfmt(EventFormat, valist));
}

void DrawEvent::Start(CommandList& InCmdList, FColor Color, const char16_t* Name)
{
	CmdList = &InCmdList;
	CmdList->PushEvent(Name, Color);
}

void DrawEvent::Stop()
{
	CmdList->PopEvent();
}