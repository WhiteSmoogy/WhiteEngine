/*! \file Engine\Render\Adapter.h
\ingroup Engine
\brief ��ʾ��������ط�װ��
*/
#ifndef WE_RENDER_D3D12_Adapter_h
#define WE_RENDER_D3D12_Adapter_h 1

#include "d3d12_dxgi.h"
#include <WBase/winttype.hpp>
#include <WFramework/WCLib/FContainer.h>

namespace platform_ex::Windows {
	namespace DXGI {
		struct VideoMode {
			UINT Width;
			UINT Height;
			DXGI_FORMAT Format;

			bool operator==(const VideoMode& video_mode) const wnoexcept {
				return (Width == video_mode.Width) && (Height == video_mode.Height) &&
					(Format == video_mode.Format); //some issue
			}

			bool operator<(const VideoMode& video_mode) const wnoexcept {
				return Width < video_mode.Width ||
					(Width == video_mode.Width &&
						(Height < video_mode.Height ||
							(Height == video_mode.Height &&
								Format < video_mode.Format)));
			}
		};

		class Adapter {
		public:
			friend class AdapterList;

			Adapter();
			Adapter(uint32 adapter_id_, COMPtr<IDXGIAdapter1> const& adapter_);

			DefGetter(wnothrow, IDXGIAdapter1*, , adapter.Get())

				string Description() const;

			size_t NumVideoMode() const;

			VideoMode GetVideoMode(size_t index) const;

			HRESULT Set(COMPtr<IDXGIAdapter1> const& adapter_);

			bool CheckHDRSupport();
		private:
			HRESULT Enumerate();
		private:
			white::uint32 adapter_id;
			COMPtr<IDXGIAdapter1> adapter;
			DXGI_ADAPTER_DESC1 adapter_desc;
			std::vector<VideoMode> video_modes;
		};

		class AdapterList : public list<Adapter>
		{
		public:
			AdapterList();

			DefGetter(const wnothrow, IDXGIFactory4*, DXGIFactory4, dxgi_factory_4.Get());

			Adapter& CurrentAdapter();

			DefGetter(const wnothrow, size_t, AdapterNum, size());
		private:
			HRESULT Enumerate();
		private:
			//todo IDXGIFactory5?
			COMPtr<IDXGIFactory4> dxgi_factory_4;

			iterator current_iterator;
		};
	}
}

#endif