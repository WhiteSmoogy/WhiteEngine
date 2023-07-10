#pragma once

#include <WBase/winttype.hpp>
#include <WBase/wmathtype.hpp>

#include <concepts>
#include <type_traits>
#include <string>

namespace std
{
	template<typename t,typename alloc>
	class vector;

	template<typename t>
	class optional;
}

namespace WhiteEngine
{

	class ArchiveState
	{
	public:
		ArchiveState()
			:ArIsLoading(false),
			ArIsSaving(false)
		{
		}

		/** Returns true if this archive is for loading data. */
		bool IsLoading() const
		{
			return ArIsLoading;
		}

		/** Returns true if this archive is for saving data, this can also be a pre-save preparation archive. */
		bool IsSaving() const
		{
			return ArIsSaving;
		}
	protected:
		/** Whether this archive is for loading data. */
		white::uint8 ArIsLoading : 1;

		/** Whether this archive is for saving data. */
		white::uint8 ArIsSaving : 1;

		/** Whether this archive contains errors, which means that further serialization is generally not safe */
		white::uint8 ArIsError : 1;
	};

	class Archive : public ArchiveState
	{
	public:
		using uint64 = white::uint64;
		using uint8 = white::uint8;
		using int8 = white::int8;
		using int64 = white::int64;
		using int32 = white::int32;

		virtual ~Archive() = default;

		Archive& operator>>(std::integral auto& v)
		{
			return Serialize(&v, sizeof(v));
		}

		Archive& operator>>(std::string& v)
		{
			std::size_t size = v.size();
			Serialize(&size, sizeof(size));
			if (IsLoading())
			{
				v.resize(size);
			}
			Serialize(v.data(), size);
			return *this;
		}

		virtual int64 Tell() const 
		{
			return 0;
		}

		virtual void Seek(int64)
		{
		}

		template<typename T> requires std::is_enum_v<T>
			Archive& operator>>(T& v)
			{
				std::underlying_type_t<T>& value = reinterpret_cast<std::underlying_type_t<T>&>(v);
				Serialize(&value, sizeof(value));
				*this;
			}
	public:
		virtual Archive& Serialize(void* v, white::uint64 length) {
			return *this;
		}
	};

	template<class T>
	concept Serializable =
		requires(Archive & archive, T & v)
	{
		archive >> v;
	};

	template<typename T>
	requires requires(Archive& archive, T& v)
	{
		{v.Serialize(archive) };
	}
	void operator>>(Archive& archive, T& v)
	{
		return v.Serialize(archive);
	}

	template<typename T>
	requires Serializable<T>&& std::default_initializable<T>
		Archive& operator>>(Archive& archive, std::vector<T>& v)
	{
		std::size_t size = v.size();
		archive >> size;
		if (archive.IsLoading())
		{
			v.resize(size);
		}
		for (int i = 0; i != v.size(); ++i)
			archive >> v[i];

		return archive;
	}

	template<typename T>
	requires Serializable<T>&& std::default_initializable<T>
		Archive& operator>>(Archive& archive, std::optional<T>& v)
	{
		bool has_value = v.has_value();
		archive >> has_value;
		if (archive.IsLoading() && has_value)
		{
			v.emplace();
			archive >> *v;
		}
		else if (has_value)
			archive >> *v;
		return archive;
	}

	template<typename T>
	requires std::integral<T>
		Archive& operator>>(Archive& archive, white::math::vector3<T>& v)
	{
		return archive.Serialize(v.begin(), sizeof(T) * 3);
	}

}