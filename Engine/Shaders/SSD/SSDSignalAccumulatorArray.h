#ifndef SSDSignalAccumulatorArray_h
#define SSDSignalAccumulatorArray_h 1

#define MAX_ACCUMULATOR_COMPRESSED_VGPRS 24

#include "SSD/SSDSignalAccumulator.h"

// Maximum number of VGPR that can be used to compress accumulator array. Currently bound by ACCUMULATOR_COMPRESSION_PENUMBRA_DRB.
#define MAX_ACCUMULATOR_COMPRESSED_VGPRS 24

struct FSSDSignalAccumulatorArray
{
	FSSDSignalAccumulator Array[SIGNAL_ARRAY_SIZE];
};

/** Accumulator array that is compressed to have lower VGPR footprint. */
struct FSSDCompressedSignalAccumulatorArray
{
	uint VGPR[MAX_ACCUMULATOR_COMPRESSED_VGPRS];
};

/** Created an initialised accumulator array. */
FSSDSignalAccumulatorArray CreateSignalAccumulatorArray()
{
	FSSDSignalAccumulatorArray Accumulators;

	[unroll(SIGNAL_ARRAY_SIZE)]
		for (uint i = 0; i < SIGNAL_ARRAY_SIZE; i++)
		{
			Accumulators.Array[i] = CreateSignalAccumulator();
		}
	return Accumulators;
}

/** Compress the accumulator array. */
FSSDCompressedSignalAccumulatorArray CompressAccumulatorArray(in FSSDSignalAccumulatorArray Accumulators, uint CompressionLayout)
{
	// Initialize the float2 arrays to be compressed at half in VGPRs.
	float2 RawArray[MAX_ACCUMULATOR_COMPRESSED_VGPRS];
	{
		[unroll(MAX_ACCUMULATOR_COMPRESSED_VGPRS)]
			for (uint i = 0; i < 2 * MAX_ACCUMULATOR_COMPRESSED_VGPRS; i++)
			{
				RawArray[i].x = 0;
				RawArray[i].y = 0;
			}
	}

	if (CompressionLayout == ACCUMULATOR_COMPRESSION_DISABLED)
	{
		// NOP
	}
#if COMPILE_DRB_ACCUMULATOR && COMPILE_MININVFREQ_ACCUMULATOR
	else if (CompressionLayout == ACCUMULATOR_COMPRESSION_PENUMBRA_DRB)
	{
		[unroll(MAX_SIGNAL_BATCH_SIZE)]
			for (uint SignalBatchId = 0; SignalBatchId < MAX_SIGNAL_BATCH_SIZE; SignalBatchId++)
			{
				const FSSDSignalAccumulator Accum = Accumulators.Array[SignalBatchId];

				RawArray[SignalBatchId * 6 + 0].x = Accum.Current.SampleCount;
				RawArray[SignalBatchId * 6 + 0].y = Accum.Current.MissCount;
				RawArray[SignalBatchId * 6 + 1].x = Accum.Current.WorldBluringRadius;
				RawArray[SignalBatchId * 6 + 1].y = Accum.CurrentInvFrequency;
				RawArray[SignalBatchId * 6 + 2].x = Accum.CurrentTranslucency;
				RawArray[SignalBatchId * 6 + 2].y = Accum.Previous.SampleCount;
				RawArray[SignalBatchId * 6 + 3].x = Accum.Previous.MissCount;
				RawArray[SignalBatchId * 6 + 3].y = Accum.Previous.WorldBluringRadius;
				RawArray[SignalBatchId * 6 + 4].x = Accum.PreviousInvFrequency;
				RawArray[SignalBatchId * 6 + 4].y = Accum.MinInvFrequency;
				RawArray[SignalBatchId * 6 + 5].x = Accum.HighestInvFrequency;
				RawArray[SignalBatchId * 6 + 5].y = Accum.BorderingRadius;
			} // for (uint SignalMultiplexId = 0; SignalMultiplexId < MAX_SIGNAL_BATCH_SIZE; SignalMultiplexId++)
	}
#endif // COMPILE_DRB_ACCUMULATOR && COMPILE_MININVFREQ_ACCUMULATOR

	// Compress.
	FSSDCompressedSignalAccumulatorArray CompressedAccumulators;
	[unroll(MAX_ACCUMULATOR_COMPRESSED_VGPRS)]
		for (uint i = 0; i < MAX_ACCUMULATOR_COMPRESSED_VGPRS; i++)
		{
			CompressedAccumulators.VGPR[i] = Compress2Floats(RawArray[i]);
		}

	return CompressedAccumulators;
} // CompressAccumulatorArray()

#endif