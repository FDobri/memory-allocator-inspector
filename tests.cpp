#include "memory.hpp"
#include <gtest/gtest.h>

TEST(Arena, AlignsAndResets) {
    memory::Arena arena(2048, 10);
    arena.allocate(3, 1);
    auto block = arena.allocate(128, 256);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(block.data) % 256, 0u);
    EXPECT_THROW(arena.allocate(2048, 1), std::bad_alloc);
    EXPECT_THROW(arena.allocate(1, 3), std::invalid_argument);
    arena.reset();
    EXPECT_EQ(arena.used(), 0u);
    const auto events = arena.journal().events();
    ASSERT_EQ(events.size(), 4u);
    EXPECT_FALSE(events.back().allocated);
    EXPECT_EQ(events[2].id, block.id);
    EXPECT_EQ(events.back().used, 0u);
}

TEST(Pool, RejectsStaleAndReusesSlots) {
    memory::Pool pool(23, 1, 64, 2);
    const auto first = pool.allocate();
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(first.data) % 64, 0u);
    EXPECT_THROW(pool.allocate(), std::bad_alloc);
    pool.release(first);
    EXPECT_THROW(pool.release(first), std::invalid_argument);
    const auto next = pool.allocate();
    EXPECT_EQ(first.data, next.data);
    EXPECT_NE(first.id, next.id);
    EXPECT_THROW(pool.release(first), std::invalid_argument);
    pool.release(next);
    const auto events = pool.journal().events();
    ASSERT_EQ(events.size(), 2u);
    EXPECT_TRUE(events.front().allocated);
    EXPECT_FALSE(events.back().allocated);
}
