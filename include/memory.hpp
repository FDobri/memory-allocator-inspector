#pragma once

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

namespace memory {
using Clock = std::chrono::steady_clock;
struct Block {
    void* data{};
    std::uint64_t id{};
};
}
