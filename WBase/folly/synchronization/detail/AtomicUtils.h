#pragma once

#include <atomic>

#include <folly/lang/Assume.h>

namespace folly {
    namespace detail {

        inline std::memory_order default_failure_memory_order(
            std::memory_order successMode) {
            switch (successMode) {
            case std::memory_order_acq_rel:
                return std::memory_order_acquire;
            case std::memory_order_release:
                return std::memory_order_relaxed;
            case std::memory_order_relaxed:
            case std::memory_order_consume:
            case std::memory_order_acquire:
            case std::memory_order_seq_cst:
                return successMode;
            }
            assume_unreachable();
        }

        inline char const* memory_order_to_str(std::memory_order mo) {
            switch (mo) {
            case std::memory_order_relaxed:
                return "relaxed";
            case std::memory_order_consume:
                return "consume";
            case std::memory_order_acquire:
                return "acquire";
            case std::memory_order_release:
                return "release";
            case std::memory_order_acq_rel:
                return "acq_rel";
            case std::memory_order_seq_cst:
                return "seq_cst";
            }
        }
    } // namespace detail
} // namespace folly