#include <type_traits>
#include <functional>

using std::invoke_result;

#include "pseudo_mutex.h"
#include "scope_gurad.hpp"
#include "cache.hpp"
#include "concurrency.h"
#include "examiner.hpp"
#include "cast.hpp"
#include "set.hpp"
#include "wmath.hpp"
#include "ConcurrentHashMap.h"

namespace wm = white::math;

static decltype(auto) foo() {

	white::ConcurrentHashMap<int, int> s{};

	return 0;
}

