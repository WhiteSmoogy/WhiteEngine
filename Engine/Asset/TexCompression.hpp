/*! \file Engine\Asset\TexCompression.hpp
\ingroup Engine\Asset
\brief Tex Compression IO ...
*/
#ifndef WE_ASSET_TEX_COMPRESSIONBC_HPP
#define WE_ASSET_TEX_COMPRESSIONBC_HPP 1

#include <WBase/winttype.hpp>
#include <WBase/operators.hpp>
#include "../emacro.h"
#include "RenderInterface/IFormat.hpp"
#include "RenderInterface/ITexture.hpp"
#include "RenderInterface/Color_T.hpp"

namespace tc {
	using namespace white::inttype;
	using namespace platform::Render::IFormat;
	using namespace platform::M;
	using namespace white::math;
	using platform::Render::TexturePtr;

	enum TexCompressionMethod
	{
		TCM_Speed,
		TCM_Balanced,
		TCM_Quality
	};

	enum TexCompressionErrorMetric
	{
		TCEM_Uniform,     // Treats r, g, and b channels equally
		TCEM_Nonuniform,  // { 0.3, 0.59, 0.11 }
	};

	class WE_API TexCompression
	{
	public:
		virtual ~TexCompression()
		{
		}

		uint32 BlockWidth() const
		{
			return block_width_;
		}
		uint32 BlockHeight() const
		{
			return block_height_;
		}
		uint32 BlockDepth() const
		{
			return block_depth_;
		}
		uint32 BlockBytes() const
		{
			return block_bytes_;
		}
		EFormat DecodedFormat() const
		{
			return decoded_fmt_;
		}

		virtual void EncodeBlock(void* output, void const * input, TexCompressionMethod method) = 0;
		virtual void DecodeBlock(void* output, void const * input) = 0;

		virtual void EncodeMem(uint32 width, uint32 height,
			void* output, uint32 out_row_pitch, uint32 out_slice_pitch,
			void const * input, uint32 in_row_pitch, uint32 in_slice_pitch,
			TexCompressionMethod method);
		virtual void DecodeMem(uint32 width, uint32 height,
			void* output, uint32 out_row_pitch, uint32 out_slice_pitch,
			void const * input, uint32 in_row_pitch, uint32 in_slice_pitch);

		virtual void EncodeTex(TexturePtr const & out_tex, TexturePtr const & in_tex, TexCompressionMethod method);

		virtual void DecodeTex(TexturePtr const & out_tex, TexturePtr const & in_tex);

	protected:
		uint32 block_width_;
		uint32 block_height_;
		uint32 block_depth_;
		uint32 block_bytes_;
		EFormat decoded_fmt_;
	};

	using TexCompressionPtr = std::shared_ptr<TexCompression>;

	class ARGBColor32 : white::equality_comparable<ARGBColor32>
	{
	public:
		enum
		{
			BChannel = 0,
			GChannel = 1,
			RChannel = 2,
			AChannel = 3
		};

	public:
		ARGBColor32()
		{
		}
		ARGBColor32(uint8 a, uint8 r, uint8 g, uint8 b)
		{
			this->a() = a;
			this->r() = r;
			this->g() = g;
			this->b() = b;
		}
		explicit ARGBColor32(uint32 dw)
		{
			clr32_.dw = dw;
		}

		uint32& ARGB()
		{
			return clr32_.dw;
		}
		uint32 ARGB() const
		{
			return clr32_.dw;
		}

		uint8& operator[](uint32 ch)
		{
			wassume(ch < 4);
			return clr32_.argb[ch];
		}
		uint8 operator[](uint32 ch) const
		{
			wassume(ch < 4);
			return clr32_.argb[ch];
		}

		uint8& a()
		{
			return (*this)[AChannel];
		}
		uint8 a() const
		{
			return (*this)[AChannel];
		}

		uint8& r()
		{
			return (*this)[RChannel];
		}
		uint8 r() const
		{
			return (*this)[RChannel];
		}

		uint8& g()
		{
			return (*this)[GChannel];
		}
		uint8 g() const
		{
			return (*this)[GChannel];
		}

		uint8& b()
		{
			return (*this)[BChannel];
		}
		uint8 b() const
		{
			return (*this)[BChannel];
		}

		bool operator==(ARGBColor32 const & rhs) const
		{
			return clr32_.dw == rhs.clr32_.dw;
		}

	private:
		union Clr32
		{
			uint32 dw;
			uint8 argb[4];
		} clr32_;
	};

	class RGBACluster
	{
		static int const MAX_NUM_DATA_POINTS = 16;

	public:
		RGBACluster(ARGBColor32 const * pixels, uint32 num,
			std::function<uint32(uint32, uint32, uint32)> const & get_partition);

		float4& Point(uint32 index)
		{
			return data_points_[point_map_[index]];
		}
		float4 const & Point(uint32 index) const
		{
			return data_points_[point_map_[index]];
		}

		ARGBColor32 const & Pixel(uint32 index) const
		{
			return data_pixels_[point_map_[index]];
		}

		uint32 NumValidPoints() const
		{
			return num_valid_points_;
		}

		float4 const & Avg() const
		{
			return avg_;
		}

		void BoundingBox(float4& min_clr, float4& max_clr) const
		{
			min_clr = min_clr_;
			max_clr = max_clr_;
		}

		bool AllSamePoint() const
		{
			return min_clr_ == max_clr_;
		}

		uint32 PrincipalAxis(float4& axis, float* eig_one, float* eig_two) const;

		void ShapeIndex(uint32 shape_index, uint32 num_partitions)
		{
			shape_index_ = shape_index;
			num_partitions_ = num_partitions;
		}

		void ShapeIndex(uint32 shape_index)
		{
			this->ShapeIndex(shape_index, num_partitions_);
		}

		void Partition(uint32 part);

		bool IsPointValid(uint32 index) const
		{
			return selected_partition_ == get_partition_(num_partitions_, shape_index_, index);
		}

	private:
		void Recalculate(bool consider_valid);
		int PowerMethod(float4x4 const & mat, float4& eig_vec, float* eig_val = nullptr) const;

	private:
		uint32 num_valid_points_;
		uint32 num_partitions_;
		uint32 selected_partition_;
		uint32 shape_index_;

		float4 avg_;

		std::array<float4, MAX_NUM_DATA_POINTS> data_points_;
		std::array<ARGBColor32, MAX_NUM_DATA_POINTS> data_pixels_;
		std::array<uint8, MAX_NUM_DATA_POINTS> point_map_;
		float4 min_clr_;
		float4 max_clr_;

		std::function<uint32(uint32, uint32, uint32)> get_partition_;
	};

	// Helpers

	inline int Mul8Bit(int a, int b)
	{
		int t = a * b + 128;
		return (t + (t >> 8)) >> 8;
	}

	inline ARGBColor32 From4Ints(int a, int r, int g, int b)
	{
		return ARGBColor32(static_cast<uint8>(clamp(a, 0, 255)),
			static_cast<uint8>(clamp(r, 0, 255)),
			static_cast<uint8>(clamp(g, 0, 255)),
			static_cast<uint8>(clamp(b, 0, 255)));
	}

	inline float4 FromARGBColor32(ARGBColor32 const & pixel)
	{
		return float4(pixel.r(), pixel.g(), pixel.b(), pixel.a());
	}

	template <int N>
	inline uint8 ExtendNTo8Bits(int input)
	{
		return static_cast<uint8>((input >> (N - (8 - N))) | (input << (8 - N)));
	}

	inline uint8 Extend4To8Bits(int input)
	{
		return ExtendNTo8Bits<4>(input);
	}

	inline uint8 Extend5To8Bits(int input)
	{
		return ExtendNTo8Bits<5>(input);
	}

	inline uint8 Extend6To8Bits(int input)
	{
		return ExtendNTo8Bits<6>(input);
	}

	inline uint8 Extend7To8Bits(int input)
	{
		return ExtendNTo8Bits<7>(input);
	}
}

#endif