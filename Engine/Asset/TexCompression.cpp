#include <WBase/smart_ptr.hpp>
#include "TexCompression.hpp"
#include "../Render/IContext.h"

namespace tc {
	using platform::Render::Texture;
	using platform::Render::Texture1D;
	using platform::Render::Texture2D;
	using platform::Render::Texture3D;
	using platform::Render::TextureCube;
	using platform::Render::Context;
	using platform::Render::Mapper;
	using TMA =platform::Render::TextureMapAccess;

	void TexCompression::EncodeMem(uint32_t width, uint32_t height,
		void* output, uint32_t out_row_pitch, uint32_t out_slice_pitch,
		void const * input, uint32_t in_row_pitch, uint32_t in_slice_pitch,
		TexCompressionMethod method)
	{
		uint32_t const elem_size = NumFormatBytes(decoded_fmt_);

		uint8_t const * src = static_cast<uint8_t const *>(input);

		std::vector<uint8_t> uncompressed(block_width_ * block_height_ * elem_size);
		for (uint32_t y_base = 0; y_base < height; y_base += block_height_)
		{
			uint8_t* dst = static_cast<uint8_t*>(output) + (y_base / block_height_) * out_row_pitch;

			for (uint32_t x_base = 0; x_base < width; x_base += block_width_)
			{
				for (uint32_t y = 0; y < block_height_; ++y)
				{
					for (uint32_t x = 0; x < block_width_; ++x)
					{
						if ((x_base + x < width) && (y_base + y < height))
						{
							memcpy(&uncompressed[(y * block_width_ + x) * elem_size],
								&src[(y_base + y) * in_row_pitch + (x_base + x) * elem_size],
								elem_size);
						}
						else
						{
							memset(&uncompressed[(y * block_width_ + x) * elem_size],
								0, elem_size);
						}
					}
				}

				this->EncodeBlock(dst, &uncompressed[0], method);
				dst += block_bytes_;
			}
		}
	}

	void TexCompression::DecodeMem(uint32_t width, uint32_t height,
		void* output, uint32_t out_row_pitch, uint32_t,
		void const * input, uint32_t in_row_pitch, uint32_t)
	{

		uint32_t const elem_size = NumFormatBytes(decoded_fmt_);

		uint8_t * dst = static_cast<uint8_t*>(output);

		std::vector<uint8_t> uncompressed(block_width_ * block_height_ * elem_size);
		for (uint32_t y_base = 0; y_base < height; y_base += block_height_)
		{
			uint8_t const * src = static_cast<uint8_t const *>(input) + in_row_pitch * (y_base / block_height_);

			uint32_t const block_h = std::min(block_height_, height - y_base);
			for (uint32_t x_base = 0; x_base < width; x_base += block_width_)
			{
				uint32_t const block_w = std::min(block_width_, width - x_base);

				this->DecodeBlock(&uncompressed[0], src);
				src += block_bytes_;

				for (uint32_t y = 0; y < block_h; ++y)
				{
					for (uint32_t x = 0; x < block_w; ++x)
					{
						memcpy(&dst[(y_base + y) * out_row_pitch + (x_base + x) * elem_size],
							&uncompressed[(y * block_width_ + x) * elem_size], elem_size);
					}
				}
			}
		}
	}

	void TexCompression::EncodeTex(TexturePtr const & out_tex_, TexturePtr const & in_tex_, TexCompressionMethod method)
	{
		auto out_tex = std::static_pointer_cast<Texture2D, Texture>(out_tex_);
		auto in_tex = std::static_pointer_cast<Texture2D, Texture>(in_tex_);


		auto width = in_tex->GetWidth(0);
		auto height = in_tex->GetHeight(0);

		decltype(out_tex) uncompressed_tex;
		if (uncompressed_tex->GetFormat() != decoded_fmt_)
		{
			uncompressed_tex =white::share_raw(Context::Instance().GetDevice().CreateTexture(width, height,
				1, 1, decoded_fmt_, EA_CPURead | EA_CPUWrite, {1,0}));
			in_tex->CopyToTexture(*uncompressed_tex);
		}
		else
		{
			uncompressed_tex = in_tex;
		}

		Mapper mapper_src(*uncompressed_tex, TMA::ReadOnly, Texture2D::Box2D(width, height));
		Mapper mapper_dst(*out_tex,TMA::WriteOnly, Texture2D::Box2D(width, height));
		this->EncodeMem(width, height, mapper_dst.Pointer<void>(), mapper_dst.RowPitch, mapper_dst.SlicePitch,
			mapper_src.Pointer<void>(), mapper_src.RowPitch, mapper_src.SlicePitch, method);
	}

	void TexCompression::DecodeTex(TexturePtr const & out_tex_, TexturePtr const & in_tex_)
	{
		auto out_tex = std::static_pointer_cast<Texture2D, Texture>(out_tex_);
		auto in_tex = std::static_pointer_cast<Texture2D, Texture>(in_tex_);

		auto width = in_tex->GetWidth(0);
		uint32_t height = in_tex->GetHeight(0);

		decltype(out_tex) decoded_tex;
		if (out_tex->GetFormat() != decoded_fmt_)
		{
			decoded_tex = white::share_raw(Context::Instance().GetDevice().CreateTexture(width, height,
				1, 1, decoded_fmt_, EA_CPURead | EA_CPUWrite, {1,0}));
		}
		else
		{
			decoded_tex = out_tex;
		}

		{
			Mapper mapper_src(*in_tex, TMA::ReadOnly, Texture2D::Box2D(width, height));
			Mapper mapper_dst(*decoded_tex, TMA::WriteOnly, Texture2D::Box2D(width, height));

			this->DecodeMem(width, height, mapper_dst.Pointer<void>(), mapper_dst.RowPitch, mapper_dst.SlicePitch,
				mapper_src.Pointer<void>(), mapper_src.RowPitch, mapper_src.SlicePitch);
		}

		if (out_tex->GetFormat() != decoded_fmt_)
		{
			decoded_tex->CopyToTexture(*out_tex);
		}
	}


	RGBACluster::RGBACluster(ARGBColor32 const * pixels, uint32_t num,
		std::function<uint32_t(uint32_t, uint32_t, uint32_t)> const & get_partition)
		: get_partition_(get_partition)
	{
		for (uint32_t i = 0; i < num; ++i)
		{
			data_pixels_[i] = pixels[i];
			data_points_[i] = FromARGBColor32(pixels[i]);
		}
		this->Recalculate(false);
	}

	// Returns the principal axis for this point cluster.
	uint32_t RGBACluster::PrincipalAxis(float4& axis, float* eig_one, float* eig_two) const
	{
		// We use these vectors for calculating the covariance matrix...
		std::array<float4, MAX_NUM_DATA_POINTS> to_pts;
		float4 to_pts_max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
			-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
		for (uint32_t i = 0; i < this->NumValidPoints(); ++i)
		{
			to_pts[i] = this->Point(i) - this->Avg();
			to_pts_max = max(to_pts_max, to_pts[i]);
		}

		// Generate a list of unique points...
		std::array<float4, MAX_NUM_DATA_POINTS> upts;
		uint32_t upts_idx = 0;
		for (uint32_t i = 0; i < this->NumValidPoints(); ++i)
		{
			bool has_pt = false;
			for (uint32_t j = 0; j < upts_idx; ++j)
			{
				if (upts[j] == this->Point(i))
				{
					has_pt = true;
					break;
				}
			}

			if (!has_pt)
			{
				upts[upts_idx] = this->Point(i);
				++upts_idx;
			}
		}

		wassume(upts_idx > 0);

		if (1 == upts_idx)
		{
			axis.x = axis.y = axis.z = axis.w = 0;
			return 0;

			// Collinear?
		}
		else
		{
			float4 dir = normalize(upts[1] - upts[0]);
			bool collinear = true;
			for (uint32_t i = 2; i < this->NumValidPoints(); ++i)
			{
				float4 v = upts[i] - upts[0];
				if (!float_equal(abs(dot(v, dir)),length(v)))
				{
					collinear = false;
					break;
				}
			}

			if (collinear)
			{
				axis = dir;
				return 0;
			}
		}

		float4x4 cov_matrix;

		// Compute covariance.
		for (uint32_t i = 0; i < 4; ++i)
		{
			for (uint32_t j = 0; j <= i; ++j)
			{
				float sum = 0;
				for (uint32_t k = 0; k < this->NumValidPoints(); ++k)
				{
					sum += to_pts[k][i] * to_pts[k][j];
				}

				cov_matrix(i, j) = sum / 3;
				cov_matrix(j, i) = cov_matrix(i, j);
			}
		}

		uint32_t iters = this->PowerMethod(cov_matrix, axis, eig_one);
		if ((eig_two != nullptr) && (eig_one != nullptr))
		{
			if (*eig_one != 0)
			{
				float4x4 reduced;
				for (uint32_t j = 0; j < 4; ++j)
				{
					for (uint32_t i = 0; i < 4; ++i)
					{
						reduced(i, j) = axis[j] * axis[i];
					}
				}

				reduced = cov_matrix - ((*eig_one) * reduced);
				bool all_zero = true;
				for (uint32_t i = 0; i < 16; ++i)
				{
					if (std::abs(reduced[i/4][i%4]) > 0.0005f)
					{
						all_zero = false;
						break;
					}
				}

				if (all_zero)
				{
					*eig_two = 0;
				}
				else
				{
					float4 dummy_dir;
					iters += this->PowerMethod(reduced, dummy_dir, eig_two);
				}
			}
			else
			{
				*eig_two = 0;
			}
		}

		return iters;
	}

	void RGBACluster::Partition(uint32_t part)
	{
		selected_partition_ = part;
		this->Recalculate(true);
	}

	void RGBACluster::Recalculate(bool consider_valid)
	{
		num_valid_points_ = 0;
		avg_ = float4(0, 0, 0, 0);
		min_clr_ = float4(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		max_clr_ = float4(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
			-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

		uint32_t map = 0;
		for (uint32_t i = 0; i < data_points_.size(); ++i)
		{
			if (consider_valid && !this->IsPointValid(i))
			{
				continue;
			}

			float4 const & p = data_points_[i];

			++num_valid_points_;
			avg_ += p;
			point_map_[map] = static_cast<uint8_t>(i);
			++map;

			min_clr_ = min(min_clr_, p);
			max_clr_ = max(max_clr_, p);
		}

		avg_ /= static_cast<float>(num_valid_points_);
	}

	// Does power iteration to determine the principal eigenvector and eigenvalue.
	// Returns them in eigVec and eigVal after kMaxNumIterations
	int RGBACluster::PowerMethod(float4x4 const & mat, float4& eig_vec, float* eig_val) const
	{
		static int const ITER_POWER = 4;

		float4 b;
		float norm = 0.5f;
		for (int i = 0; i < 4; ++i)
		{
			b[i] = norm;
		}

		bool bad_eigen_value = false;
		bool fixed = false;
		int num_iterations = 1;
		while (!fixed && (num_iterations < ITER_POWER))
		{
			float4 new_b = transform(b, mat);

			// !HACK! If the principal eigenvector of the matrix
			// converges to zero, that could mean that there is no
			// principal eigenvector. However, that may be due to
			// poor initialization of the random vector, so rerandomize
			// and try again.
			float const new_b_len = length(new_b);
			if (new_b_len < 1e-6f)
			{
				if (bad_eigen_value)
				{
					eig_vec = b;
					if (eig_val)
					{
						*eig_val = 0;
					}
					return num_iterations;
				}

				for (int i = 0; i < 2; ++i)
				{
					b[i] = 1;
				}

				b = normalize(b);
				bad_eigen_value = true;
				continue;
			}

			new_b = normalize(new_b);

			// If the new eigenvector is close enough to the old one,
			// then we've converged.
			if (float_equal(1.0f, dot(b, new_b)))
			{
				fixed = true;
			}

			// Save and continue.
			b = new_b;

			++num_iterations;
		}

		// Store the eigenvector in the proper variable.
		eig_vec = b;

		// Store eigenvalue if it was requested
		if (eig_val)
		{
			float4 result = transform(b, mat);
			*eig_val = length(result) / length(b);
		}

		return num_iterations;
	}
}