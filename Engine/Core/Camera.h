/*! \file Core\Camera.h
\ingroup Engine
\brief 提供相机基础数据类型。
*/

#ifndef WE_Core_Camera_H
#define WE_Core_Camera_H 1

#include <WBase/wmath.hpp>
#include <cmath>

namespace WhiteEngine {
	namespace X {

		namespace wm = white::math;
		inline wm::float4x4 perspective_fov_lh(float fov, float aspect, float nearPlane, float farPlane) noexcept {
			auto h = 1 / std::tanf(fov / 2);
			auto w = h / aspect;
			auto q = (farPlane / (farPlane - nearPlane));
			return {
				{ w,0,0,0 },
				{ 0,h,0,0 },
				{ 0,0,q,1 },
				{ 0,0,-nearPlane * q,0 }
			};
		}

		inline wm::float4x4 look_at_lh(const wm::float3& eye, const wm::float3& at, const wm::float3& up) noexcept {
			auto z = wm::normalize(at - eye);
			auto x = wm::normalize(wm::cross(up, z));
			auto y = wm::cross(z, x);

			return {
				{ x.x,y.x,z.x,0 },
				{ x.y,y.y,z.y,0 },
				{ x.z,y.z,z.z,0 },
				{ -wm::dot(x,eye),-wm::dot(y,eye),-wm::dot(z,eye),1 }
			};
		}

		
	}
}

namespace WhiteEngine::Core {
	namespace wm = white::math;

	//TODO!
	//我们应该使用艺术工作者所使用工具的坐标系[3D-MAX]
	//! "right-handed" coordinate systems, where the positive X-Axis points
	//! to the right, the positive Y-Axis points away from the viewer and the positive
	//! Z-Axis points up. The following illustration shows our coordinate system.
	//! <PRE>
	//! z-axis
	//!  ^
	//!  |
	//!  |   y-axis
	//!  |  /
	//!  | /
	//!  |/
	//!  +---------------->   x-axis
	//! </PRE>

	class Camera {
	public:
		class TransformElement {
		public:
			TransformElement()
				:matrix(wm::float4x4::identity)
			{
			}

			wm::float3 GetEyePos() const noexcept {
				return  reinterpret_cast<const wm::float3&>(matrix[3].xyz);
			}
			void SetEyePos(wm::float3 pos) noexcept {
				matrix[3].xyz = pos;
			}

			wm::float3 GetRightVector() const noexcept {
				return  reinterpret_cast<const wm::float3&>(matrix[0].xyz);
			}
			void SetRightVector(wm::float3 right) noexcept {
				matrix[0].xyz = right;
			}
			wm::float3 GetUpVector() const noexcept {
				return  reinterpret_cast<const wm::float3&>(matrix[1].xyz);
			}
			void SetUpVector(wm::float3 up) noexcept {
				matrix[1].xyz = up;
			}

			wm::float3 GetForwardVector() const noexcept {
				return  reinterpret_cast<const wm::float3&>(matrix[2].xyz);
			}
			void SetForwardVector(wm::float3 forward) noexcept {
				matrix[2].xyz = forward;
			}

			wm::float4x4 GetViewMatrix() const noexcept {
				return wm::inverse(matrix);
			}

			void SetViewMatrix(const wm::float4x4& view) noexcept {
				matrix = wm::inverse(view);
			}
		private:
			wm::float4x4  matrix; // world-space matrix:translation view postiion to world
		};

		//Frustum Top Point in coordinate origin
		class FrustumElement {
		private:
			float fov = 75.f/180*3.14f;
			float aspect = 1;
			float nearPlane = 0.25f;
			float farPlane = 1024.f;
		};
	public:
		wm::float3 GetEyePos() const noexcept {
			return transform_element.GetEyePos();
		}
		void SetEyePos(wm::float3 pos) noexcept {
			transform_element.SetEyePos(pos);
		}

		wm::float3 GetRightVector() const noexcept {
			return transform_element.GetRightVector();
		}
		void SetRightVector(wm::float3 right) noexcept {
			transform_element.SetRightVector(right);
		}
		wm::float3 GetUpVector() const noexcept {
			return  transform_element.GetUpVector();
		}
		void SetUpVector(wm::float3 up) noexcept {
			transform_element.SetUpVector(up);
		}

		wm::float3 GetForwardVector() const noexcept {
			return transform_element.GetForwardVector();
		}
		void SetForwardVector(wm::float3 forward) noexcept {
			transform_element.SetForwardVector(forward);
		}

		wm::float4x4 GetViewMatrix() const noexcept {
			return transform_element.GetViewMatrix();
		}

		void SetViewMatrix(const wm::float4x4& view) noexcept {
			transform_element.SetViewMatrix(view);
		}

		void SetFrustum(white::uint16 width, white::uint16 height, const FrustumElement& frustum = {});

		white::uint16 GetFrustumViewWidth() const { return width; }
		white::uint16 GetFrustumViewHeight() const { return height; }
	protected:
		TransformElement transform_element;
		FrustumElement frustum_elemnt;

		white::uint16 width;
		white::uint16 height;
	};
}


#endif