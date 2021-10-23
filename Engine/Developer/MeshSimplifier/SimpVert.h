#pragma once

#include "CoreTypes.h"
#include "Developer/MeshBuild.h"

namespace WhiteEngine {
	// Example vertex type for TMeshSimplifier
	template<uint32 NumTexCoords >
	class TVertSimp
	{
		typedef TVertSimp< NumTexCoords > VertType;
	public:
		wm::float3			Position;
		wm::float3			Normal;
		wm::float3			Tangents[2];
		LinearColor	Color;
		wm::float2		TexCoords[NumTexCoords];

		wm::float3& GetPos() { return Position; }
		const wm::float3& GetPos() const { return Position; }
		float* GetAttributes() { return (float*)&Normal; }
		const float* GetAttributes() const { return (const float*)&Normal; }

		void		Correct()
		{
			Normal = wm::normalize(Normal);
			Tangents[0] -= dot(Tangents[0], Normal) * Normal;
			Tangents[0] = wm::normalize(Tangents[0]);
			Tangents[1] -= dot(Tangents[1], Normal) * Normal;
			Tangents[1] -= dot(Tangents[1], Tangents[0]) * Tangents[0];
			Tangents[1] = wm::normalize(Tangents[1]);
			Color = clamp(Color);
		}

		bool		Equals(const VertType& a) const
		{
			if (!PointsEqual(Position, a.Position) ||
				!NormalsEqual(Tangents[0], a.Tangents[0]) ||
				!NormalsEqual(Tangents[1], a.Tangents[1]) ||
				!NormalsEqual(Normal, a.Normal) ||
				!ColorsEqual(Color,a.Color))
			{
				return false;
			}

			// UVs
			for (int32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++)
			{
				if (!UVsEqual(TexCoords[UVIndex], a.TexCoords[UVIndex]))
				{
					return false;
				}
			}

			return true;
		}

		bool		operator==(const VertType& a) const
		{
			if (Position != a.Position ||
				Normal != a.Normal ||
				Tangents[0] != a.Tangents[0] ||
				Tangents[1] != a.Tangents[1] ||
				Color != a.Color)
			{
				return false;
			}

			for (uint32 i = 0; i < NumTexCoords; i++)
			{
				if (TexCoords[i] != a.TexCoords[i])
				{
					return false;
				}
			}
			return true;
		}

		VertType	operator+(const VertType& a) const
		{
			VertType v;
			v.Position = Position + a.Position;
			v.Normal = Normal + a.Normal;
			v.Tangents[0] = Tangents[0] + a.Tangents[0];
			v.Tangents[1] = Tangents[1] + a.Tangents[1];
			v.Color = Color + a.Color;

			for (uint32 i = 0; i < NumTexCoords; i++)
			{
				v.TexCoords[i] = TexCoords[i] + a.TexCoords[i];
			}
			return v;
		}

		VertType	operator-(const VertType& a) const
		{
			VertType v;
			v.Position = Position - a.Position;
			v.Normal = Normal - a.Normal;
			v.Tangents[0] = Tangents[0] - a.Tangents[0];
			v.Tangents[1] = Tangents[1] - a.Tangents[1];
			v.Color = Color - a.Color;

			for (uint32 i = 0; i < NumTexCoords; i++)
			{
				v.TexCoords[i] = TexCoords[i] - a.TexCoords[i];
			}
			return v;
		}

		VertType	operator*(const float a) const
		{
			VertType v;
			v.Position = Position * a;
			v.Normal = Normal * a;
			v.Tangents[0] = Tangents[0] * a;
			v.Tangents[1] = Tangents[1] * a;
			v.Color = Color * a;

			for (uint32 i = 0; i < NumTexCoords; i++)
			{
				v.TexCoords[i] = TexCoords[i] * a;
			}
			return v;
		}

		VertType	operator/(const float a) const
		{
			float ia = 1.0f / a;
			return (*this) * ia;
		}
	};
}