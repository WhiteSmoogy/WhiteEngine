/*! \file Engine\Render\PipeState.h
\ingroup Engine\Render
\important It's have default state
*/

#ifndef WE_RENDER_PIPESTATE_h
#define WE_RENDER_PIPESTATE_h 1

#include "TextureSampleDesc.h"

#include <cctype>
#include <algorithm>
#include <compare>

namespace platform::Render {
	namespace details {
		template<typename T, typename Y>
		T union_type_as(Y v) {
			union Y2T
			{
				Y y;
				T t;
			} y2t;
			y2t.y = v;
			return y2t.t;
		}
	}



	enum class RasterizerMode {
		Point,
		Line,
		Face,
	};

	enum class CullMode {
		None,
		Front,
		Back
	};

	enum class BlendOp
	{
		Add = 1,
		Sub,
		Rev_Sub,
		Min,
		Max
	};

	enum class BlendFactor
	{
		Zero,
		One,
		Src_Alpha,
		Dst_Alpha,
		Inv_Src_Alpha,
		Inv_Dst_Alpha,
		Src_Color,
		Dst_Color,
		Inv_Src_Color,
		Inv_Dst_Color,
		Src_Alpha_Sat,
		Factor,
		Inv_Factor,
#if     1
		Src1_Alpha,
		Inv_Src1_Alpha,
		Src1_Color,
		Inv_Src1_Color
#endif
	};

	enum class StencilOp
	{
		Keep,
		Zero,
		Replace,
		Incr,//clamp
		Decr,
		Invert,//bits invert
		Incr_Wrap,
		Decr_Wrap
	};

	enum class ColorMask : white::uint8
	{
		Red = 1UL << 0,
		Green = 1UL << 1,
		Blue = 1UL << 2,
		Alpha = 1UL << 3,
		All = Red | Green | Blue | Alpha

	};

	

	enum class AsyncComputeBudget
	{
		All_4 = 4, //Async compute can use the entire GPU.
	};

	

	struct RasterizerDesc
	{
		RasterizerMode mode;
		CullMode cull;
		bool ccw;
		bool depth_clip_enable;
		bool scissor_enable;
		bool multisample_enable;

		RasterizerDesc();
		friend auto operator<=>(const RasterizerDesc& lhs, const RasterizerDesc& rhs) = default;
		friend bool operator==(const RasterizerDesc& lhs, const RasterizerDesc& rhs) = default;

		template<typename T>
		static T to_mode(const std::string & value);


	
	};

	template<>
	inline RasterizerMode   RasterizerDesc::to_mode<RasterizerMode>(const std::string & value) {
		auto hash = white::constfn_hash(value.c_str());
		switch (hash)
		{
		case white::constfn_hash("Point"):
			return RasterizerMode::Point;
		case white::constfn_hash("Line"):
			return RasterizerMode::Line;
		case white::constfn_hash("Face"):
			return RasterizerMode::Face;
		}

		throw white::narrowing_error();
	}

	template<>
	inline CullMode RasterizerDesc::to_mode<CullMode>(const std::string & value)
	{
		auto hash = white::constfn_hash(value.c_str());
		switch (hash)
		{
		case white::constfn_hash("None"):
			return CullMode::None;
		case white::constfn_hash("Front"):
			return CullMode::Front;
		case white::constfn_hash("Back"):
			return CullMode::Back;
		}

		throw white::narrowing_error();
	}

	struct DepthStencilDesc
	{
		bool				depth_enable;
		bool				depth_write_mask;
		CompareOp		depth_func;

		bool				front_stencil_enable;
		CompareOp		front_stencil_func;
		white::uint16			front_stencil_ref;
		white::uint16			front_stencil_read_mask;
		white::uint16			front_stencil_write_mask;
		StencilOp	front_stencil_fail;
		StencilOp	front_stencil_depth_fail;
		StencilOp	front_stencil_pass;

		bool				back_stencil_enable;
		CompareOp		back_stencil_func;
		white::uint16			back_stencil_ref;
		white::uint16			back_stencil_read_mask;
		white::uint16			back_stencil_write_mask;
		StencilOp	back_stencil_fail;
		StencilOp	back_stencil_depth_fail;
		StencilOp	back_stencil_pass;

		DepthStencilDesc();

		friend auto operator<=>(const DepthStencilDesc& lhs, const DepthStencilDesc& rhs) = default;
		friend bool operator==(const DepthStencilDesc& lhs, const DepthStencilDesc& rhs) = default;

		template<typename T>
		static T to_op(const std::string & value);
	};


	template<>
	inline CompareOp DepthStencilDesc::to_op<CompareOp>(const std::string & value)
	{
		auto lower_value = value;
		white::to_lower(lower_value);
		auto hash = white::constfn_hash(lower_value.c_str());
		switch (hash) {
		case white::constfn_hash("fail"):
			return CompareOp::Fail;
		case white::constfn_hash("pass"):
			return CompareOp::Pass;
		case white::constfn_hash("less"):
			return CompareOp::Less;
		case white::constfn_hash("less_equal"):
			return CompareOp::Less_Equal;
		case white::constfn_hash("equal"):
			return CompareOp::Equal;
		case white::constfn_hash("notequal"):
			return CompareOp::NotEqual;
		case white::constfn_hash("greaterequal"):
			return CompareOp::GreaterEqual;
		case white::constfn_hash("greater"):
			return CompareOp::Greater;
		}

		throw white::narrowing_error();
	}

	template<>
	inline StencilOp DepthStencilDesc::to_op<StencilOp>(const std::string& value)
	{
		auto lower_value = value;
		white::to_lower(lower_value);
		auto hash = white::constfn_hash(lower_value.c_str());
		switch (hash) {
		case white::constfn_hash("keep"):
			return StencilOp::Keep;
		case white::constfn_hash("zero"):
			return StencilOp::Zero;
		case white::constfn_hash("replace"):
			return StencilOp::Replace;
		case white::constfn_hash("decr"):
			return StencilOp::Decr;
		case white::constfn_hash("invert"):
			return StencilOp::Invert;
		case white::constfn_hash("incr_wrap"):
			return StencilOp::Incr_Wrap;
		case white::constfn_hash("decr_wrap"):
			return StencilOp::Decr_Wrap;
		}

		throw white::narrowing_error();
	}

	struct BlendDesc
	{
		LinearColor blend_factor;
		uint32_t sample_mask;

		bool				alpha_to_coverage_enable;
		bool				independent_blend_enable;

		std::array<bool, 8>				blend_enable;
		std::array<bool, 8>				logic_op_enable;
		std::array<BlendOp, 8>	blend_op;
		std::array<BlendFactor, 8>	src_blend;
		std::array<BlendFactor, 8>	dst_blend;
		std::array<BlendOp, 8>	blend_op_alpha;
		std::array<BlendFactor, 8>	src_blend_alpha;
		std::array<BlendFactor, 8>	dst_blend_alpha;
		std::array<white::uint8, 8>			color_write_mask;

		BlendDesc();

		friend auto operator<=>(const BlendDesc& lhs, const BlendDesc& rhs) = default;
		friend bool operator==(const BlendDesc& lhs, const BlendDesc& rhs) = default;


		static BlendFactor to_factor(const std::string& value)
		{
			auto hash = white::constfn_hash(value.c_str());
			switch (hash) {
			case white::constfn_hash("Zero"):
				return BlendFactor::Zero;
			case white::constfn_hash("One"):
				return BlendFactor::One;
			case white::constfn_hash("Src_Alpha"):
				return BlendFactor::Src_Alpha;
			case white::constfn_hash("Dst_Alpha"):
				return BlendFactor::Dst_Alpha;
			case white::constfn_hash("Inv_Src_Alpha"):
				return BlendFactor::Inv_Src_Alpha;
			case white::constfn_hash("Inv_Dst_Alpha"):
				return BlendFactor::Inv_Dst_Alpha;
			case white::constfn_hash("Src_Color"):
				return BlendFactor::Src_Color;
			case white::constfn_hash("Dst_Color"):
				return BlendFactor::Dst_Color;
			case white::constfn_hash("Inv_Src_Color"):
				return BlendFactor::Inv_Src_Color;
			case white::constfn_hash("Inv_Dst_Color"):
				return BlendFactor::Inv_Dst_Color;
			case white::constfn_hash("Src_Alpha_Sat"):
				return BlendFactor::Src_Alpha_Sat;
			case white::constfn_hash("Factor"):
				return BlendFactor::Factor;
			case white::constfn_hash("Inv_Factor"):
				return BlendFactor::Inv_Factor;
#if     1			
			case white::constfn_hash("Src1_Alpha"):
				return BlendFactor::Src1_Alpha;
			case white::constfn_hash("Inv_Src1_Alpha"):
				return BlendFactor::Inv_Src1_Alpha;
			case white::constfn_hash("Src1_Color"):
				return BlendFactor::Src1_Color;
			case white::constfn_hash("Inv_Src1_Color"):
				return BlendFactor::Inv_Src1_Color;
#endif
			}
			throw white::narrowing_error();
		}

		static BlendOp to_op(const std::string& value) {
			auto hash = white::constfn_hash(value.c_str());
			switch (hash) {
			case white::constfn_hash("Add"):
				return BlendOp::Add;
			case white::constfn_hash("Sub"):
				return BlendOp::Sub;
			case white::constfn_hash("Rev_Sub"):
				return BlendOp::Rev_Sub;
			case white::constfn_hash("Min"):
				return BlendOp::Min;
			case white::constfn_hash("Max"):
				return BlendOp::Max;
			}
			throw white::narrowing_error();
		}
	};

	class PipleState {
	public:
		explicit PipleState(const RasterizerDesc r_desc = {}, const DepthStencilDesc& ds_desc = {},
			const BlendDesc& b_desc = {})
			:RasterizerState(r_desc), DepthStencilState(ds_desc), BlendState(b_desc)
		{}

		virtual ~PipleState();

	public:
		RasterizerDesc RasterizerState;
		DepthStencilDesc DepthStencilState;
		BlendDesc BlendState;
	};

}

#endif