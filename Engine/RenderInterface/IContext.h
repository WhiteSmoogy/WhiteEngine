/*! \file Engine\Render\IContext.h
\ingroup Engine
*/
#ifndef WE_RENDER_IContext_h
#define WE_RENDER_IContext_h 1

#include "IDevice.h"
#include "IDisplay.h"

namespace platform::Render {

	class FrameBuffer;

	class RayContext;
	class CommandContext;
	class CommandList;

	class Context
 {
	public:
		virtual Device& GetDevice() = 0;
		virtual Display& GetDisplay() = 0;

		virtual RayContext& GetRayContext() = 0;

		virtual void Render(CommandList& CmdList,const Effect::Effect&, const Effect::Technique&, const InputLayout&) = 0;


		virtual void BeginFrame() = 0;

		virtual CommandContext* GetDefaultCommandContext() = 0;

		virtual void AdvanceFrameFence() = 0;
		virtual void AdvanceDisplayBuffer() = 0;
	public:
		virtual void CreateDeviceAndDisplay(DisplaySetting setting) = 0;
	private:
		virtual void DoBindFrameBuffer(const std::shared_ptr<FrameBuffer>&) = 0;
	public:
		void SetFrame(const std::shared_ptr<FrameBuffer>&);
		const std::shared_ptr<FrameBuffer>& GetCurrFrame() const;
		const std::shared_ptr<FrameBuffer>& GetScreenFrame() const;

	public:
		static Context& Instance();
	protected:
		std::shared_ptr<FrameBuffer> curr_frame_buffer;
		std::shared_ptr<FrameBuffer> screen_frame_buffer;
	};
}

extern platform::Render::Context* GRenderIF;

#endif