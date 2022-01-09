/*! \file Engine\Render\InputLayout.hpp
\ingroup Engine
\brief ����������ط�װ��
*/
#ifndef WE_RENDER_D3D12_InputLayout_hpp
#define WE_RENDER_D3D12_InputLayout_hpp 1

#include "d3d12_dxgi.h"
#include "../InputLayout.hpp"
#include "../IDevice.h"

#include <vector>

namespace platform_ex::Windows::D3D12 {
	class InputLayout : public platform::Render::InputLayout {
	public:
		InputLayout();
		~InputLayout() override = default;

		const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputDesc() const;

		void Active() const;
	private:
		 mutable std::vector<D3D12_INPUT_ELEMENT_DESC> vertex_elems;
	};
}

#endif 