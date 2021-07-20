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

class Arena {
public:
    explicit Arena(std::size_t capacity, std::size_t history = 0)
        : data_(new std::byte[capacity]), capacity_(capacity), journal_(history),
          trace_(history != 0) {}
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Block allocate(std::size_t bytes, std::size_t alignment) {
        if (!bytes || !std::has_single_bit(alignment))
            throw std::invalid_argument("size and alignment must be positive; alignment must be a power of two");
        void* pointer = data_.get() + used_;
        auto remaining = capacity_ - used_;
        if (!std::align(alignment, bytes, pointer, remaining)) throw std::bad_alloc();
        auto offset = static_cast<std::size_t>(static_cast<std::byte*>(pointer) - data_.get());
        auto id = journal_.next_id();
        if (trace_) live_.push_back({id, offset, bytes, offset + bytes, {}, true});
        used_ = offset + bytes;
        journal_.add(id, offset, bytes, used_, true);
        return {pointer, id};
    }
    void reset() noexcept {
        for (auto it = live_.rbegin(); it != live_.rend(); ++it) {
            const auto next = it + 1;
            journal_.add(it->id, it->offset, it->bytes, next == live_.rend() ? 0 : next->used, false);
        }
        live_.clear();
        used_ = 0;
    }
    std::size_t used() const noexcept { return used_; }
    const Journal& journal() const noexcept { return journal_; }
private:
    std::unique_ptr<std::byte[]> data_;
    std::size_t capacity_, used_{};
    Journal journal_;
    bool trace_;
    std::vector<Event> live_;
};

class Pool {
public:
    Pool(std::size_t bytes, std::size_t count, std::size_t alignment, std::size_t history = 0)
        : stride_(stride(bytes, alignment)), alignment_(std::max(alignment, alignof(std::max_align_t))),
          ids_(checked_count(count, stride_)), journal_(history) {
        free_.reserve(count);
        for (auto i = count; i > 0; --i) free_.push_back(i - 1);
        data_ = static_cast<std::byte*>(::operator new(stride_ * count, std::align_val_t(alignment_)));
    }
    ~Pool() { ::operator delete(data_, std::align_val_t(alignment_)); }
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    Block allocate() {
        if (free_.empty()) throw std::bad_alloc();
        auto index = free_.back();
        auto id = journal_.next_id();
        free_.pop_back();
        ids_[index] = id;
        journal_.add(id, index * stride_, stride_, used(), true);
        return {data_ + index * stride_, id};
    }
    void release(Block block) {
        auto address = reinterpret_cast<std::uintptr_t>(block.data);
        auto base = reinterpret_cast<std::uintptr_t>(data_);
        if (address < base) throw std::invalid_argument("foreign block");
        auto offset = address - base;
        auto index = offset / stride_;
        if (offset % stride_ || index >= ids_.size() || !block.id || ids_[index] != block.id)
            throw std::invalid_argument("foreign or stale block");
        ids_[index] = 0;
        free_.push_back(index);
        journal_.add(block.id, offset, stride_, used(), false);
    }
    std::size_t used() const noexcept { return (ids_.size() - free_.size()) * stride_; }
    const Journal& journal() const noexcept { return journal_; }
private:
    static std::size_t stride(std::size_t bytes, std::size_t alignment) {
        if (!bytes || !std::has_single_bit(alignment) ||
            bytes > std::numeric_limits<std::size_t>::max() - (alignment - 1))
            throw std::invalid_argument("invalid slot size or alignment");
        return (bytes + alignment - 1) & ~(alignment - 1);
    }
    static std::size_t checked_count(std::size_t count, std::size_t stride) {
        if (!count || count > std::numeric_limits<std::size_t>::max() / stride)
            throw std::invalid_argument("invalid slot count");
        return count;
    }
    std::size_t stride_, alignment_;
    std::vector<std::uint64_t> ids_;
    std::vector<std::size_t> free_;
    Journal journal_;
    std::byte* data_{};
};
}
