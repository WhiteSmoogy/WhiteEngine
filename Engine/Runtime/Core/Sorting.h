#pragma once
#include <WBase/wdef.h>
#include <WBase/winttype.hpp>
#include <memory>
namespace WhiteEngine
{
	/**
	* Very fast 32bit radix sort.
	* SortKeyClass defines operator() that takes ValueType and returns a uint32. Sorting based on key.
	* No comparisons. Is stable.
	 * Use a smaller CountType for smaller histograms.
	 */
	template< typename ValueType, typename CountType, class SortKeyClass >
	void RadixSort32(ValueType* wrestrict Dst, ValueType* wrestrict Src, CountType Num, SortKeyClass SortKey)
	{
		CountType Histograms[1024 + 2048 + 2048];
		CountType* wrestrict Histogram0 = Histograms + 0;
		CountType* wrestrict Histogram1 = Histogram0 + 1024;
		CountType* wrestrict Histogram2 = Histogram1 + 2048;

		std::memset(Histograms, 0,sizeof(Histograms));

		{
			// Parallel histogram generation pass
			const ValueType* wrestrict s = (const ValueType * wrestrict)Src;
			for (CountType i = 0; i < Num; i++)
			{
				white::uint32 Key = SortKey(s[i]);
				Histogram0[(Key >> 0) & 1023]++;
				Histogram1[(Key >> 10) & 2047]++;
				Histogram2[(Key >> 21) & 2047]++;
			}
		}
		{
			// Prefix sum
			// Set each histogram entry to the sum of entries preceding it
			CountType Sum0 = 0;
			CountType Sum1 = 0;
			CountType Sum2 = 0;
			for (CountType i = 0; i < 1024; i++)
			{
				CountType t;
				t = Histogram0[i] + Sum0; Histogram0[i] = Sum0 - 1; Sum0 = t;
				t = Histogram1[i] + Sum1; Histogram1[i] = Sum1 - 1; Sum1 = t;
				t = Histogram2[i] + Sum2; Histogram2[i] = Sum2 - 1; Sum2 = t;
			}
			for (CountType i = 1024; i < 2048; i++)
			{
				CountType t;
				t = Histogram1[i] + Sum1; Histogram1[i] = Sum1 - 1; Sum1 = t;
				t = Histogram2[i] + Sum2; Histogram2[i] = Sum2 - 1; Sum2 = t;
			}
		}
		{
			// Sort pass 1
			const ValueType* wrestrict s = (const ValueType * wrestrict)Src;
			ValueType* wrestrict d = Dst;
			for (CountType i = 0; i < Num; i++)
			{
				ValueType Value = s[i];
				white::uint32 Key = SortKey(Value);
				d[++Histogram0[((Key >> 0) & 1023)]] = Value;
			}
		}
		{
			// Sort pass 2
			const ValueType* wrestrict s = (const ValueType * wrestrict)Dst;
			ValueType* wrestrict d = Src;
			for (CountType i = 0; i < Num; i++)
			{
				ValueType Value = s[i];
				white::uint32 Key = SortKey(Value);
				d[++Histogram1[((Key >> 10) & 2047)]] = Value;
			}
		}
		{
			// Sort pass 3
			const ValueType* wrestrict s = (const ValueType * wrestrict)Src;
			ValueType* wrestrict d = Dst;
			for (CountType i = 0; i < Num; i++)
			{
				ValueType Value = s[i];
				white::uint32 Key = SortKey(Value);
				d[++Histogram2[((Key >> 21) & 2047)]] = Value;
			}
		}
	}
}