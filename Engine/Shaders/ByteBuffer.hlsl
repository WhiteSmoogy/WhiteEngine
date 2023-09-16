#define RESOURCE_TYPE_FLOAT4_BUFFER				(0)
#define RESOURCE_TYPE_STRUCTURED_BUFFER			(1)
#define RESOURCE_TYPE_UINT_BUFFER				(2)
#define RESOURCE_TYPE_UINT4_ALIGNED_BUFFER		(3)
#define RESOURCE_TYPE_FLOAT4_TEXTURE			(4)

#define STRUCTURED_ELEMENT_SIZE_UINT1 (0)
#define STRUCTURED_ELEMENT_SIZE_UINT2 (1)
#define STRUCTURED_ELEMENT_SIZE_UINT4 (2)

uint Value;
uint Size;
uint SrcOffset;
uint DstOffset;
uint Float4sPerLine;

#if STRUCTURED_ELEMENT_SIZE_UINT1 == STRUCTURED_ELEMENT_SIZE
	#define STRUCTURED_BUFFER_ACCESS(BufferName, Index) BufferName##1x[(Index)]
	#define STRUCTURED_VALUE Value
#elif STRUCTURED_ELEMENT_SIZE_UINT2 == STRUCTURED_ELEMENT_SIZE
	#define STRUCTURED_BUFFER_ACCESS(BufferName, Index) BufferName##2x[(Index)]
	#define STRUCTURED_VALUE uint2(Value, Value)
#elif STRUCTURED_ELEMENT_SIZE_UINT4 == STRUCTURED_ELEMENT_SIZE
	#define STRUCTURED_BUFFER_ACCESS(BufferName, Index) BufferName##4x[(Index)]
	#define STRUCTURED_VALUE uint4(Value,Value,Value,Value)
#endif

#if RESOURCE_TYPE == RESOURCE_TYPE_FLOAT4_BUFFER
	StructuredBuffer<uint>			ScatterStructuredBuffer;
	Buffer<float4>					SrcBuffer;
	StructuredBuffer<float4>		UploadStructuredBuffer4x;
	RWBuffer<float4>				DstBuffer;
#elif RESOURCE_TYPE == RESOURCE_TYPE_STRUCTURED_BUFFER
	StructuredBuffer<uint>			ScatterStructuredBuffer;

	StructuredBuffer<uint>		UploadStructuredBuffer1x;
	StructuredBuffer<uint2>		UploadStructuredBuffer2x;
	StructuredBuffer<uint4>		UploadStructuredBuffer4x;

	StructuredBuffer<uint>		SrcStructuredBuffer1x;
	StructuredBuffer<uint2>		SrcStructuredBuffer2x;
	StructuredBuffer<uint4>		SrcStructuredBuffer4x;

	RWStructuredBuffer<uint>	DstStructuredBuffer1x;
	RWStructuredBuffer<uint2>	DstStructuredBuffer2x;
	RWStructuredBuffer<uint4>	DstStructuredBuffer4x;
	
#elif RESOURCE_TYPE == RESOURCE_TYPE_FLOAT4_TEXTURE
	StructuredBuffer<uint>			ScatterStructuredBuffer;
	Texture2D<float4>				SrcTexture;
	StructuredBuffer<float4>		UploadStructuredBuffer4x;
	RWTexture2D<float4>				DstTexture;
#elif RESOURCE_TYPE == RESOURCE_TYPE_UINT_BUFFER || RESOURCE_TYPE == RESOURCE_TYPE_UINT4_ALIGNED_BUFFER
	ByteAddressBuffer				ScatterByteAddressBuffer;
	ByteAddressBuffer				SrcByteAddressBuffer;
	ByteAddressBuffer				UploadByteAddressBuffer;
	RWByteAddressBuffer				DstByteAddressBuffer;
#endif

[numthreads(64, 1, 1)]
void MemcpyCS( uint ThreadId : SV_DispatchThreadID ) 
{
#if RESOURCE_TYPE == RESOURCE_TYPE_FLOAT4_BUFFER
	// Size is in float4s
	if (ThreadId < Size)
	{
		DstBuffer[DstOffset + ThreadId] = SrcBuffer[SrcOffset + ThreadId];
	}
#elif RESOURCE_TYPE == RESOURCE_TYPE_STRUCTURED_BUFFER
	// Size is in float4s
	if( ThreadId < Size )
	{
		STRUCTURED_BUFFER_ACCESS(DstStructuredBuffer, DstOffset + ThreadId) = STRUCTURED_BUFFER_ACCESS(SrcStructuredBuffer, SrcOffset + ThreadId);
	}
	#elif RESOURCE_TYPE == RESOURCE_TYPE_FLOAT4_TEXTURE
	uint2 IndexTexture;
	IndexTexture.y = ThreadId / (Float4sPerLine);
	IndexTexture.x = ThreadId % (Float4sPerLine);

	if(ThreadId < Size)
	{
		float4 SrcValue = SrcTexture.Load(float3(IndexTexture.x, IndexTexture.y, 0));
		DstTexture[IndexTexture.xy] = SrcValue;
	}
#elif RESOURCE_TYPE == RESOURCE_TYPE_UINT_BUFFER || RESOURCE_TYPE == RESOURCE_TYPE_UINT4_ALIGNED_BUFFER
	// Size and offsets are in dwords
	uint SrcIndex = SrcOffset + ThreadId * 4;
	uint DstIndex = DstOffset + ThreadId * 4;

	if( ThreadId * 4 + 3 < Size )
	{
		uint4 SrcData = SrcByteAddressBuffer.Load4( SrcIndex * 4 );
		DstByteAddressBuffer.Store4( DstIndex * 4, SrcData );
	}
	else if( ThreadId * 4 + 2 < Size )
	{
		uint3 SrcData = SrcByteAddressBuffer.Load3( SrcIndex * 4 );
		DstByteAddressBuffer.Store3( DstIndex * 4, SrcData );
	}
	else if( ThreadId * 4 + 1 < Size )
	{
		uint2 SrcData = SrcByteAddressBuffer.Load2( SrcIndex * 4 );
		DstByteAddressBuffer.Store2( DstIndex * 4, SrcData );
	}
	else if( ThreadId * 4 < Size )
	{
		uint SrcData = SrcByteAddressBuffer.Load( SrcIndex * 4 );
		DstByteAddressBuffer.Store( DstIndex * 4, SrcData );
	}
#else
	#error "Not implemented"
#endif
}

[numthreads(64, 1, 1)]
void MemsetCS( uint ThreadId : SV_DispatchThreadID ) 
{
#if RESOURCE_TYPE == RESOURCE_TYPE_FLOAT4_BUFFER
	// Size is in float4s
	if (ThreadId < Size)
	{
		DstBuffer[DstOffset + ThreadId] = asfloat(Value);
	}
#elif RESOURCE_TYPE == RESOURCE_TYPE_STRUCTURED_BUFFER
	// Size is in float4s
	if( ThreadId < Size )
	{
		STRUCTURED_BUFFER_ACCESS(DstStructuredBuffer, DstOffset + ThreadId) = STRUCTURED_VALUE;
	}
#elif RESOURCE_TYPE == RESOURCE_TYPE_FLOAT4_TEXTURE
	uint2 IndexTexture;
	IndexTexture.y = ThreadId / (Float4sPerLine);
	IndexTexture.x = ThreadId % (Float4sPerLine);

	if(ThreadId < Size)
	{
		DstTexture[IndexTexture.xy] = float4(asfloat(Value),asfloat(Value),asfloat(Value),asfloat(Value));
	}
#elif RESOURCE_TYPE == RESOURCE_TYPE_UINT_BUFFER || RESOURCE_TYPE == RESOURCE_TYPE_UINT4_ALIGNED_BUFFER
	// Size and offsets are in dwords
	uint SrcIndex = SrcOffset + ThreadId * 4;
	uint DstIndex = DstOffset + ThreadId * 4;

	if( ThreadId * 4 + 3 < Size )
	{
		uint4 SrcData = uint4( Value, Value, Value, Value);
		DstByteAddressBuffer.Store4( DstIndex * 4, SrcData );
	}
	else if( ThreadId * 4 + 2 < Size )
	{
		uint3 SrcData = uint3( Value, Value, Value);
		DstByteAddressBuffer.Store3( DstIndex * 4, SrcData );
	}
	else if( ThreadId * 4 + 1 < Size )
	{
		uint2 SrcData = uint2( Value, Value);
		DstByteAddressBuffer.Store2( DstIndex * 4, SrcData );
	}
	else if( ThreadId * 4 < Size )
	{
		uint SrcData = Value;
		DstByteAddressBuffer.Store( DstIndex * 4, SrcData );
	}
#else
	#error Not implemented
#endif
}