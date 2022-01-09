#pragma once

#include <map>
#include <unordered_map>

namespace white
{
	template<class K, class V, class H, class E,class A>
	V* find(std::unordered_map<K, V, H, E, A>& map, const K& k)
	{
		auto itr = map.find(k);
		if (itr != map.end())
			return &(itr->second);
		return nullptr;
	}
}