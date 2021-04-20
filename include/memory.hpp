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
struct Event {
    std::uint64_t id{};
    std::size_t offset{}, bytes{}, used{};
    Clock::time_point time;
    bool allocated{};
};

class Journal {
public:
    explicit Journal(std::size_t limit) : events_(limit), start_(Clock::now()) {}
    std::uint64_t next_id() {
        if (id_ == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("allocation ID exhausted");
        return ++id_;
    }
    void add(std::uint64_t id, std::size_t offset, std::size_t bytes,
             std::size_t used, bool allocated) noexcept {
        if (events_.empty()) return;
        events_[next_] = {id, offset, bytes, used, Clock::now(), allocated};
        next_ = (next_ + 1) % events_.size();
        size_ = std::min(size_ + 1, events_.size());
    }
    std::vector<Event> events() const {
        std::vector<Event> result;
        result.reserve(size_);
        if (size_ == 0) return result;
        auto first = (next_ + events_.size() - size_) % events_.size();
        for (std::size_t i = 0; i < size_; ++i)
            result.push_back(events_[(first + i) % events_.size()]);
        return result;
    }
    Clock::time_point start() const noexcept { return start_; }
private:
    std::vector<Event> events_;
    Clock::time_point start_;
    std::size_t next_{}, size_{};
    std::uint64_t id_{};
};

}
