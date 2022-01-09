/*! \file Engine\Render\Color_T.hpp
\ingroup Engine
*/

#ifndef WE_RENDER_ColorT_hpp
#define WE_RENDER_ColorT_hpp 1

#include "../emacro.h"
#include "Engine/CoreTypes.h"

#include <utility>

namespace WhiteEngine
{
	inline float linear_to_srgb(float linear) wnoexcept
	{
		if (linear < 0.0031308f)
		{
			return 12.92f * linear;
		}
		else
		{
			float const ALPHA = 0.055f;
			return (1 + ALPHA) * pow(linear, 1 / 2.4f) - ALPHA;
		}
	}

	inline float srgb_to_linear(float srgb) wnoexcept
	{
		if (srgb < 0.04045f)
		{
			return srgb / 12.92f;
		}
		else
		{
			float const ALPHA = 0.055f;
			return pow((srgb + ALPHA) / (1 + ALPHA), 2.4f);
		}
	}

	namespace details
	{
		constexpr float f = 1 / 255.f;

		template<typename type_struct, bool anti = false>
		struct vector_4d_impl {
			using component_type = typename type_struct::component_type;
			using vector3d_type = typename type_struct::vector3d_type;
			using vector4d_type = typename type_struct::vector4d_type;
			using scalar_type = component_type;
			using vec_type = vector_4d_impl;

			union {
				wm::type_details::component<type_struct, 0> r;
				wm::type_details::component<type_struct, 1> g;
				wm::type_details::component<type_struct, 2> b;
				wm::type_details::component<type_struct, 3> a;

				wm::type_details::sub_vector2d<type_struct, anti, 0, 1> rg;
				wm::type_details::sub_vector2d<type_struct, anti, 0, 2> rb;
				wm::type_details::sub_vector2d<type_struct, anti, 0, 3> ra;
				wm::type_details::sub_vector2d<type_struct, anti, 1, 0> gr;
				wm::type_details::sub_vector2d<type_struct, anti, 1, 2> gb;
				wm::type_details::sub_vector2d<type_struct, anti, 1, 3> ga;
				wm::type_details::sub_vector2d<type_struct, anti, 2, 0> br;
				wm::type_details::sub_vector2d<type_struct, anti, 2, 1> bg;
				wm::type_details::sub_vector2d<type_struct, anti, 2, 3> ba;
				wm::type_details::sub_vector2d<type_struct, anti, 3, 0> ar;
				wm::type_details::sub_vector2d<type_struct, anti, 3, 1> ag;
				wm::type_details::sub_vector2d<type_struct, anti, 3, 2> ab;

				wm::type_details::sub_vector3d<type_struct, anti, 0, 1, 2> rgb;
				wm::type_details::sub_vector3d<type_struct, anti, 0, 2, 1> rbg;
				wm::type_details::sub_vector3d<type_struct, anti, 0, 1, 3> rga;
				wm::type_details::sub_vector3d<type_struct, anti, 0, 3, 1> rag;
				wm::type_details::sub_vector3d<type_struct, anti, 0, 2, 3> rba;
				wm::type_details::sub_vector3d<type_struct, anti, 0, 3, 2> rab;
				wm::type_details::sub_vector3d<type_struct, anti, 1, 0, 2> grb;
				wm::type_details::sub_vector3d<type_struct, anti, 1, 2, 0> gbr;
				wm::type_details::sub_vector3d<type_struct, anti, 1, 0, 3> gra;
				wm::type_details::sub_vector3d<type_struct, anti, 1, 3, 0> gar;
				wm::type_details::sub_vector3d<type_struct, anti, 1, 2, 3> gba;
				wm::type_details::sub_vector3d<type_struct, anti, 1, 3, 2> gab;
				wm::type_details::sub_vector3d<type_struct, anti, 2, 0, 1> brg;
				wm::type_details::sub_vector3d<type_struct, anti, 2, 1, 0> bgr;
				wm::type_details::sub_vector3d<type_struct, anti, 2, 0, 3> bra;
				wm::type_details::sub_vector3d<type_struct, anti, 2, 3, 0> bar;
				wm::type_details::sub_vector3d<type_struct, anti, 2, 1, 3> bga;
				wm::type_details::sub_vector3d<type_struct, anti, 2, 3, 1> bag;
				wm::type_details::sub_vector3d<type_struct, anti, 3, 0, 1> arg;
				wm::type_details::sub_vector3d<type_struct, anti, 3, 1, 0> agr;
				wm::type_details::sub_vector3d<type_struct, anti, 3, 0, 2> arb;
				wm::type_details::sub_vector3d<type_struct, anti, 3, 2, 0> abr;
				wm::type_details::sub_vector3d<type_struct, anti, 3, 1, 2> agb;
				wm::type_details::sub_vector3d<type_struct, anti, 3, 2, 1> abg;

				wm::type_details::sub_vector4d<type_struct, anti, 0, 1, 2, 3>  rgba;
				wm::type_details::sub_vector4d<type_struct, anti, 0, 1, 3, 2>  rgab;
				wm::type_details::sub_vector4d<type_struct, anti, 0, 2, 1, 3>  rbga;
				wm::type_details::sub_vector4d<type_struct, anti, 0, 2, 3, 1>  rbag;
				wm::type_details::sub_vector4d<type_struct, anti, 0, 3, 1, 2>  ragb;
				wm::type_details::sub_vector4d<type_struct, anti, 0, 3, 2, 1>  rabg;
				wm::type_details::sub_vector4d<type_struct, anti, 1, 0, 2, 3>  grba;
				wm::type_details::sub_vector4d<type_struct, anti, 1, 0, 3, 2>  grab;
				wm::type_details::sub_vector4d<type_struct, anti, 1, 2, 0, 3>  gbra;
				wm::type_details::sub_vector4d<type_struct, anti, 1, 2, 3, 0>  gbar;
				wm::type_details::sub_vector4d<type_struct, anti, 1, 3, 0, 2>  garb;
				wm::type_details::sub_vector4d<type_struct, anti, 1, 3, 2, 0>  gabr;
				wm::type_details::sub_vector4d<type_struct, anti, 2, 0, 1, 3>  brga;
				wm::type_details::sub_vector4d<type_struct, anti, 2, 0, 3, 1>  brag;
				wm::type_details::sub_vector4d<type_struct, anti, 2, 1, 0, 3>  bgra;
				wm::type_details::sub_vector4d<type_struct, anti, 2, 1, 3, 0>  bgar;
				wm::type_details::sub_vector4d<type_struct, anti, 2, 3, 0, 1>  barg;
				wm::type_details::sub_vector4d<type_struct, anti, 2, 3, 1, 0>  bagr;
				wm::type_details::sub_vector4d<type_struct, anti, 3, 0, 1, 2>  argb;
				wm::type_details::sub_vector4d<type_struct, anti, 3, 0, 2, 1>  arbg;
				wm::type_details::sub_vector4d<type_struct, anti, 3, 1, 0, 2>  agrb;
				wm::type_details::sub_vector4d<type_struct, anti, 3, 1, 2, 0>  agbr;
				wm::type_details::sub_vector4d<type_struct, anti, 3, 2, 0, 1>  abrg;
				wm::type_details::sub_vector4d<type_struct, anti, 3, 2, 1, 0>  abgr;

				scalar_type data[4];
			};

			constexpr vector_4d_impl(scalar_type x, scalar_type y, scalar_type z, scalar_type w) noexcept
				:data{ x,y ,z ,w }
			{
			}

			constexpr vector_4d_impl(vector4d_type xyzw) noexcept
				:data{ xyzw.x,xyzw.y ,xyzw.z ,xyzw.w }
			{
			}

			constexpr vector_4d_impl(vector3d_type xyz, scalar_type w) noexcept
				:data{ xyz.x,xyz.y,xyz.z,w }
			{
			}

			constexpr explicit vector_4d_impl(uint32 dw) noexcept
				:data{ 
				 f* (static_cast<float>(static_cast<uint8>(dw >> 16))),
				 f* (static_cast<float>(static_cast<uint8>(dw >> 8))),
				 f* (static_cast<float>(static_cast<uint8>(dw >> 0))),
				 f* (static_cast<float>(static_cast<uint8>(dw >> 24)))
				}
			{}

			constexpr vector_4d_impl() noexcept = default;

			constexpr size_t size() const {
				return 4;
			}

			const scalar_type& operator[](std::size_t index) const noexcept {
				wassume(index < size());
				return data[index];
			}

			scalar_type& operator[](std::size_t index) noexcept {
				wassume(index < size());
				return data[index];
			}

			const constexpr scalar_type* begin() const noexcept {
				return data + 0;
			}

			const constexpr scalar_type* end() const noexcept {
				return data + size();
			}

			scalar_type* begin() noexcept {
				return data + 0;
			}

			scalar_type* end() noexcept {
				return data + size();
			}

			vector_4d_impl& operator=(const vector_4d_impl& rhs)
			{
				r = rhs.r;
				g = rhs.g;
				b = rhs.b;
				a = rhs.a;

				return *this;
			}

			constexpr uint32 ARGB() const wnoexcept
			{
				auto R = static_cast<uint8>(white::math::saturate(r) * 255 + 0.5f);
				auto G = static_cast<uint8>(white::math::saturate(g) * 255 + 0.5f);
				auto B = static_cast<uint8>(white::math::saturate(b) * 255 + 0.5f);
				auto A = static_cast<uint8>(white::math::saturate(a) * 255 + 0.5f);

				return (A << 24) | (R << 16) | (G << 8) | (B << 0);
			}

			friend auto constexpr operator<=>(const vector_4d_impl& lhs, const vector_4d_impl& rhs) noexcept {
				return std::lexicographical_compare_three_way(lhs.begin(),lhs.end(),rhs.begin(),rhs.end());
			}

			friend bool operator==(const vector_4d_impl& lhs, const vector_4d_impl& rhs) noexcept {
				return lhs.ARGB() == rhs.ARGB();
			}
		};
	}

	namespace type_details
	{
		template<typename scalar, typename vector2d_impl, typename vector3d_impl, typename vector4d_impl>
		using color_4d = details::vector_4d_impl<wm::type_details::type_vector4d<scalar, vector2d_impl, vector3d_impl, vector4d_impl>>;
	}

	using LinearColor = type_details::color_4d<float, wm::float2, wm::float3, wm::float4>;


	inline LinearColor modulate(LinearColor lhs, LinearColor  rhs) wnoexcept
	{
		return  { lhs.rgba * rhs.rgba };
	}

	inline LinearColor  lerp(LinearColor lhs, LinearColor rhs, float w) {
		return { wm::lerp<float>(lhs.rgba, rhs.rgba, w) };
	}

	inline float dot(LinearColor lhs, LinearColor rhs) {
		return dot(lhs.rgba, rhs.rgba);
	}

	inline LinearColor clamp(LinearColor color, float InMin = 0.0f, float InMax = 1.0f)
	{
		return wm::clamp<wm::float4>(color.rgba, wm::float4(InMin, InMin, InMin, InMin), wm::float4(InMax, InMax, InMax, InMax));
	}

	void ConvertFromABGR32F(EFormat fmt, LinearColor const* input, uint32 num_elems, void* output);
	void ConvertToABGR32F(EFormat fmt, void const* input, uint32_t num_elems, LinearColor* output);

	template<EFormat format>
	struct TColor;

	template<>
	struct TColor<EF_ARGB8_SRGB>
	{
		using uint8 = white::uint8;

		union { struct { uint8 B, G, R, A; }; white::uint32 AlignmentDummy; };

		TColor() :AlignmentDummy(0) {}

		constexpr TColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255)
			: B(InB), G(InG), R(InR), A(InA)
		{}

		white::uint32& DWColor() { return *((white::uint32*)this); }
		const white::uint32& DWColor() const { return *((white::uint32*)this); }
	};

	template<>
	struct TColor<EF_ABGR32F> :LinearColor
	{};

	//	Stores a color with 8 bits of precision per channel.
	using FColor = TColor<EF_ARGB8_SRGB>;

	inline LinearColor as_linear(const FColor& color)
	{
		return { color.R / 255.f,color.G / 255.f,color.B / 255.f,color.A / 255.f };
	}

	inline FColor as_color(const LinearColor& color)
	{
		float FloatR = wm::clamp<float>(color.r, 0.0f, 1.0f);
		float FloatG = wm::clamp<float>(color.g, 0.0f, 1.0f);
		float FloatB = wm::clamp<float>(color.b, 0.0f, 1.0f);
		float FloatA = wm::clamp<float>(color.a, 0.0f, 1.0f);

		FColor ret;

		ret.A = (uint8)std::lround(FloatA * 255.0f);
		ret.R = (uint8)std::lround(FloatR * 255.0f);
		ret.G = (uint8)std::lround(FloatG * 255.0f);
		ret.B = (uint8)std::lround(FloatB * 255.0f);

		return ret;
	}
}

namespace platform
{
	using LinearColor = WhiteEngine::LinearColor;
	using FColor = WhiteEngine::FColor;

	namespace M {
		using WhiteEngine::ConvertFromABGR32F;
		using WhiteEngine::ConvertToABGR32F;
	}
}

#endif