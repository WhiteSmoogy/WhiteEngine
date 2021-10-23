#ifndef WBase_LMATHQUATERNION_HPP
#define WBase_LMATHQUATERNION_HPP 1

#include "type_traits.hpp"
#include "operators.hpp"
#include "wmathtype.hpp"

namespace white::math {
	template<typename scalar>
	struct walignas(16) basic_quaternion final :private
		addable<basic_quaternion<scalar>>,
		subtractable<basic_quaternion<scalar>>,
		dividable<basic_quaternion<scalar>, scalar>,
		multipliable<basic_quaternion<scalar>>,
		multipliable<basic_quaternion<scalar>, scalar>,
		equality_comparable<basic_quaternion<scalar>>
	{
		static_assert(is_floating_point<scalar>::value);

		constexpr size_t size() const {
			return 4;
		}

		constexpr basic_quaternion() noexcept = default;

		constexpr basic_quaternion(const basic_quaternion&)  noexcept = default;
		constexpr basic_quaternion(basic_quaternion&&)  noexcept = default;

		constexpr basic_quaternion(scalar x, scalar y, scalar z, scalar w) noexcept
			:x(x),y(y),z(z),w(w)
		{}

		constexpr basic_quaternion(const vector3<scalar>& xyz,scalar w) noexcept
			:x(xyz.x),y(xyz.y),z(xyz.z),w(w)
		{}

		basic_quaternion& operator=(const basic_quaternion&) noexcept = default;
		basic_quaternion& operator=(basic_quaternion&&) noexcept = default;

		const static basic_quaternion identity;

		constexpr const scalar* begin() const noexcept {
			return data + 0;
		}

		constexpr const scalar* end() const noexcept {
			return data + size();
		}

		scalar* begin() noexcept {
			return data + 0;
		}

		scalar* end() noexcept {
			return data + size();
		}

		constexpr const scalar& operator[](std::size_t index) const noexcept {
			wassume(index < size());
			return data[index];
		}

		scalar& operator[](std::size_t index) noexcept {
			wassume(index < size());
			return data[index];
		}

		friend constexpr basic_quaternion<scalar> operator+(basic_quaternion<scalar> lhs, basic_quaternion<scalar> rhs) noexcept;
		friend constexpr basic_quaternion<scalar> operator-(basic_quaternion<scalar> lhs, basic_quaternion<scalar> rhs) noexcept;
		friend constexpr basic_quaternion<scalar> operator*(basic_quaternion<scalar> lhs, basic_quaternion<scalar> rhs) noexcept;
		friend constexpr basic_quaternion<scalar> operator*(basic_quaternion<scalar> lhs, scalar rhs) noexcept;
		friend constexpr basic_quaternion<scalar> operator/(basic_quaternion<scalar> lhs, scalar rhs) noexcept;

		constexpr basic_quaternion operator+() const noexcept;
		constexpr basic_quaternion operator-() const noexcept;

		constexpr bool operator==(const basic_quaternion& rhs) const noexcept;

		union {
			struct {
				scalar x, y, z, w;
			};
			scalar data[4];
		};
	};

	template<typename scalar>
	const basic_quaternion<scalar> basic_quaternion<scalar>::identity{ 0,0,0,1 };

	using quaternion = basic_quaternion<float>;
}

#endif