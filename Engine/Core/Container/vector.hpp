#pragma once

#include <vector>
#include <algorithm>
namespace white
{
	constexpr int INDEX_NONE = -1;

	template<typename T,typename Allocator>
	std::size_t add_unique(std::vector<T, Allocator>& vec, T&& args)
	{
		auto itr = std::find(vec.begin(), vec.end(), args);
		if (itr != vec.end())
			return itr - vec.begin();

		auto index = vec.size();
		vec.emplace_back(std::forward<T>(args));
		return index;
	}

	template<typename T, typename Allocator>
	std::size_t add_unique(std::vector<T, Allocator>& vec,const T& args)
	{
		auto itr = std::find(vec.begin(), vec.end(), args);
		if (itr != vec.end())
			return itr - vec.begin();

		auto index = vec.size();
		vec.emplace_back(args);
		return index;
	}

	template<typename T, typename Allocator>
	int find_index(std::vector<T, Allocator>& vec, const T& args)
	{
		auto itr = std::find(vec.begin(), vec.end(), args);
		if (itr != vec.end())
			return static_cast<int>(itr - vec.begin());

		return INDEX_NONE;
	}

	template<typename T, typename Allocator>
	inline T pop(std::vector<T, Allocator>& vec)
	{
		wconstraint(!vec.empty());
		auto last = vec.back();
		vec.resize(vec.size()-1);
		return last;
	}

	template<typename T, typename Allocator>
	void remove_at_swap(std::vector<T, Allocator>& vec, int32 Index)
	{
		std::swap(vec[Index], vec[vec.size() - 1]);
		vec.pop_back();
	}

	template<typename T, typename XAllocator,  typename YAllocator>
	void append(std::vector<T, XAllocator>& range, std::vector<T, YAllocator>& nextrange)
	{
		range.insert(range.end(), nextrange.begin(), nextrange.end());
	}
}