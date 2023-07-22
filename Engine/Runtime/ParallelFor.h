#pragma once

#include <WBase/winttype.hpp>
#include <WBase/enum.hpp>
#include <ranges>
#include <algorithm>
#include <execution>
namespace WhiteEngine
{
	namespace details
	{
		class iota_iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = white::int32;
			using difference_type = white::int32;

			using pointer = white::int32*;
			using reference = white::int32&;

			iota_iterator()
				:I(0)
			{}

			white::int32 operator*() const {
				return I;
			}

			iota_iterator& operator++()
			{
				++I;
				return *this;
			}

			friend bool
				operator==(const iota_iterator& x, const iota_iterator& y) = default;
		private:
			white::int32 I;
		};
	}

	template<typename Functor>
	requires std::is_invocable_v<Functor,white::int32>
	inline void ParallelFor(white::int32 Num, Functor Body)
	{
		std::for_each_n(std::execution::par_unseq, details::iota_iterator{}, Num, Body);
	}

	template<typename ExPo,typename Functor>
	requires std::is_invocable_v<Functor, white::int32>
		inline void ParallelFor(ExPo&& policy, white::int32 Num, Functor Body)
	{
		std::for_each_n(std::forward<ExPo>(policy), details::iota_iterator{}, Num, Body);
	}

	enum class ParallelForFlags
	{
		None,

		SingleThread
	};

	template<typename Functor>
	requires std::is_invocable_v<Functor, white::int32>
		inline void ParallelFor(white::int32 Num, Functor Body, ParallelForFlags Flags)
	{
		if (white::has_anyflags(Flags, ParallelForFlags::SingleThread))
		{
			for (int i = 0; i != Num; ++i)
			{
				Body(i);
			}
		}
		else {
			std::for_each_n(std::execution::par_unseq, details::iota_iterator{}, Num, Body);
		}
	}
}