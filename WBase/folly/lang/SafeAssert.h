#pragma once
#include <cassert.h>

#define FOLLY_SAFE_CHECK(expr, ...)  WAssert(expr,"Assertion failure.")
#define FOLLY_SAFE_DCHECK FOLLY_SAFE_CHECK