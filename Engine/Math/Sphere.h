#pragma once
#include <WBase/wmath.hpp>


namespace WhiteEngine
{
	namespace wm = white::math;

	struct Sphere
	{
		wm::float3 Center;

		float W;

		Sphere()
			:Center(0,0,0)
			,W(0)
		{

		}

		Sphere(wm::float3 center,float r)
			:Center(center)
			, W(r)
		{
		}

		Sphere(const wm::float3* Points, int32 Num)
		{
			wconstraint(Num > 0);

			// Min/max points of AABB
			int32 MinIndex[3] = { 0, 0, 0 };
			int32 MaxIndex[3] = { 0, 0, 0 };

			for (int i = 0; i < Num; i++)
			{
				for (int k = 0; k < 3; k++)
				{
					MinIndex[k] = Points[i][k] < Points[MinIndex[k]][k] ? i : MinIndex[k];
					MaxIndex[k] = Points[i][k] > Points[MaxIndex[k]][k] ? i : MaxIndex[k];
				}
			}

			float LargestDistSqr = 0.0f;
			int32 LargestAxis = 0;
			for (int k = 0; k < 3; k++)
			{
				wm::float3 PointMin = Points[MinIndex[k]];
				wm::float3 PointMax = Points[MaxIndex[k]];

				float DistSqr = wm::length_sq(PointMax - PointMin);
				if (DistSqr > LargestDistSqr)
				{
					LargestDistSqr = DistSqr;
					LargestAxis = k;
				}
			}

			wm::float3 PointMin = Points[MinIndex[LargestAxis]];
			wm::float3 PointMax = Points[MaxIndex[LargestAxis]];

			Center = 0.5f * (PointMin + PointMax);
			W = 0.5f * std::sqrt(LargestDistSqr);

			// Adjust to fit all points
			for (int i = 0; i < Num; i++)
			{
				float DistSqr = wm::length_sq(Points[i] - Center);

				if (DistSqr > W * W)
				{
					float Dist = std::sqrt(DistSqr);
					float t = 0.5f + 0.5f * (W / Dist);

					Center = wm::lerp(Points[i], Center, t);
					W = 0.5f * (W + Dist);
				}
			}
		}

		/**
		 * Constructor.
		 *
		* @param Pts Pointer to list of points this sphere must contain.
		* @param Count How many points are in the list.
		*/
		Sphere(const Sphere* Spheres, int32 Num)
		{
			wconstraint(Num > 0);

			// Min/max points of AABB
			int32 MinIndex[3] = { 0, 0, 0 };
			int32 MaxIndex[3] = { 0, 0, 0 };

			for (int i = 0; i < Num; i++)
			{
				for (int k = 0; k < 3; k++)
				{
					MinIndex[k] = Spheres[i].Center[k] - Spheres[i].W < Spheres[MinIndex[k]].Center[k] - Spheres[MinIndex[k]].W ? i : MinIndex[k];
					MaxIndex[k] = Spheres[i].Center[k] + Spheres[i].W > Spheres[MaxIndex[k]].Center[k] + Spheres[MaxIndex[k]].W ? i : MaxIndex[k];
				}
			}

			float LargestDist = 0.0f;
			int32 LargestAxis = 0;
			for (int k = 0; k < 3; k++)
			{
				Sphere SphereMin = Spheres[MinIndex[k]];
				Sphere SphereMax = Spheres[MaxIndex[k]];

				float Dist = wm::length(SphereMax.Center - SphereMin.Center) + SphereMin.W + SphereMax.W;
				if (Dist > LargestDist)
				{
					LargestDist = Dist;
					LargestAxis = k;
				}
			}

			*this = Spheres[MinIndex[LargestAxis]];
			*this += Spheres[MaxIndex[LargestAxis]];

			// Adjust to fit all spheres
			for (int i = 0; i < Num; i++)
			{
				*this += Spheres[i];
			}
		}

		bool IsInside(const Sphere& Other, float Tolerance = wm::KindaEpsilon) const
		{
			if (W > Other.W + Tolerance)
			{
				return false;
			}

			return wm::length_sq(Center - Other.Center) <= wm::Square(Other.W + Tolerance - W);
		}

		Sphere& operator+=(const Sphere& Other)
		{
			if (W == 0.f)
			{
				*this = Other;
				return *this;
			}

			auto ToOther = Other.Center - Center;
			float DistSqr = wm::length_sq(ToOther);

			if (wm::Square(W - Other.W) + wm::KindaEpsilon >= DistSqr)
			{
				// Pick the smaller
				if (W < Other.W)
				{
					*this = Other;
				}
			}
			else
			{
				float Dist = std::sqrt(DistSqr);

				Sphere NewSphere;
				NewSphere.W = (Dist + Other.W + W) * 0.5f;
				NewSphere.Center = Center;

				if (Dist > wm::SmallNumber)
				{
					NewSphere.Center += ToOther * ((NewSphere.W - W) / Dist);
				}

				// make sure both are inside afterwards
				wconstraint(Other.IsInside(NewSphere, 1.f));
				wconstraint(IsInside(NewSphere, 1.f));

				*this = NewSphere;
			}

			return *this;
		}

		friend bool operator==(const Sphere& l, const Sphere& r)
		{
			return (l.Center == r.Center) && wm::float_equal(l.W, r.W);
		}
	};
}