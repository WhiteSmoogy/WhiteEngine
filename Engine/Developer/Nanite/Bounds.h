// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

struct FBounds
{
	wm::float3	Min = {  wm::MAX_flt,  wm::MAX_flt,  wm::MAX_flt };
	wm::float3	Max = { -wm::MAX_flt, -wm::MAX_flt, -wm::MAX_flt };

	inline FBounds& operator=( const wm::float3& Other )
	{
		Min = Other;
		Max = Other;
		return *this;
	}

	inline FBounds& operator+=( const wm::float3& Other )
	{
		Min = wm::min( Min, Other );
		Max = wm::max( Max, Other );
		return *this;
	}

	inline FBounds& operator+=( const FBounds& Other )
	{
		Min = wm::min( Min, Other.Min );
		Max = wm::max( Max, Other.Max );
		return *this;
	}

	inline FBounds operator+( const FBounds& Other ) const
	{
		return FBounds(*this) += Other;
	}

	inline wm::float3 GetCenter() const
	{
		return (Max + Min) * 0.5f;
	}

	inline wm::float3 GetExtent() const
	{
		return (Max - Min) * 0.5f;
	}

	inline float GetSurfaceArea() const
	{
		wm::float3 Size = Max - Min;
		return 0.5f * ( Size.x * Size.y + Size.x * Size.z + Size.y * Size.z );
	}

	template<typename FArchive>
	inline friend FArchive& operator<<(FArchive& Ar, FBounds& Bounds)
	{
		Ar << Bounds.Min;
		Ar << Bounds.Max;
		return Ar;
	}
};