#pragma once

#include <vector>
#include <bitset>
#include "Math/PlatformMath.h"
namespace white
{
	template<typename Allocator = std::allocator<bool>>
	class TBitArray
	{
		using WordType = unsigned int;
		static int constexpr Bits = sizeof(WordType) * 8;

		class BitReference
		{
		public:
			constexpr BitReference(WordType& InData, WordType InMask)
				:Data(InData), Mask(InMask)
			{
			}

			constexpr operator bool() const
			{
				return (Data & Mask) != 0;
			}

			void operator=(const bool NewValue)
			{
				if (NewValue)
				{
					Data |= Mask;
				}
				else
				{
					Data &= ~Mask;
				}
			}

			void operator|=(const bool NewValue)
			{
				if (NewValue)
				{
					Data |= Mask;
				}
			}
			void operator&=(const bool NewValue)
			{
				if (!NewValue)
				{
					Data &= ~Mask;
				}
			}
		private:
			WordType& Data;
			WordType Mask;
		};

		class ConstBitReference
		{
		public:
			constexpr ConstBitReference(const WordType& InData, WordType InMask)
				:Data(InData), Mask(InMask)
			{
			}

			constexpr operator bool() const
			{
				return (Data & Mask) != 0;
			}
		private:
			const WordType& Data;
			WordType Mask;
		};
	public:
		using size_type = int;

		constexpr TBitArray()
			:vec(),bits(0)
		{}

		constexpr TBitArray(bool val, size_type newsize)
			:TBitArray()
		{
			resize(newsize, val);
		}

		constexpr void reserve(size_type size)
		{
			vec.reserve(wm::DivideAndRoundUp(size, Bits));
		}

		constexpr void resize(size_type newsize, bool val = false)
		{
			if (size() < newsize)
			{
				vec.resize(wm::DivideAndRoundUp(newsize, Bits), val ? (WordType)-1 : 0);

				auto mod = size() % Bits;
				if(val)
					vec[size() / Bits] |= (WordType)-1 - ((1 << mod) - 1);
				else
					vec[size() / Bits] &= ((1 << mod) - 1);

			}
			else if (newsize < size())
			{
				vec.resize(wm::DivideAndRoundUp(newsize, Bits));
			}
			bits = newsize;
		}

		[[nodiscard]]
		constexpr size_type size() const noexcept {
			return bits;
		}

		constexpr BitReference operator[](unsigned int index)
		{
			return { vec[index / Bits] ,1u << (index % Bits) };
		}

		constexpr ConstBitReference operator[](unsigned int  index) const
		{
			return ConstBitReference(vec[index / Bits] ,1u << (index % Bits));
		}

		
		void emplace_back(bool val)
		{
			resize(size() + 1, val);
		}

		constexpr WordType* data(){
			return vec.data();
		}
	public:
		std::vector<WordType,typename std::allocator_traits<Allocator>::template rebind_alloc<WordType>> vec;
		size_type bits;
	};

	template<typename Allocator = std::allocator<bool>>
	class TConstSetBitIterator
	{
	public:
		TConstSetBitIterator(const TBitArray<Allocator>& InArray)
			:Array(InArray),CurrentBitIndex(-1){
			FindFirstSetBit();
		}

		TConstSetBitIterator& operator++()
		{
			// Find the first set bit that hasn't been visited yet.
			FindFirstSetBit();

			return *this;
		}

		explicit operator bool() const
		{
			return CurrentBitIndex < Array.size();
		}

		/** Index accessor. */
		int32 GetIndex() const
		{
			return CurrentBitIndex;
		}
	private:
		const TBitArray<Allocator>& Array;
		int32 CurrentBitIndex;

		void FindFirstSetBit()
		{
			for (; ++CurrentBitIndex < Array.size();)
			{
				if (Array[CurrentBitIndex])
					break;
			}
		}
	};

}