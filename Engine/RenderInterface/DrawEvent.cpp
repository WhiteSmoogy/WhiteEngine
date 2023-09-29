#include "DrawEvent.h"
#include <cstdarg>
#include <WFramework/Core/WString.h>

#if USE_PIX
#include <WFramework/WCLib/NativeAPI.h>
#include <WinPixEventRuntime/pix3.h>
#endif

using namespace platform::Render;

platform::Render::DrawEventName::DrawEventName(const char* EventFormat, ...)
{
	va_list valist;
	va_start(valist, EventFormat);
	FormatedEventName = white::Text::String(white::vsfmt(EventFormat, valist));
}

void DrawEvent::Start(CommandList& InCmdList, FColor Color, const char16_t* Name)
{
#if USE_PIX
	PIXBeginEvent(PIX_COLOR(Color.R, Color.G, Color.B), reinterpret_cast<const wchar_t*>(Name));
#endif
	CmdList = &InCmdList;
	CmdList->PushEvent(Name, Color);
}

void DrawEvent::Stop()
{
	CmdList->PopEvent();
#if USE_PIX
	PIXEndEvent();
#endif
}