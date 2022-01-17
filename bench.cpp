#include "memory.hpp"
#include <array>
#include <benchmark/benchmark.h>

namespace {
constexpr std::size_t bytes = 64;
constexpr std::size_t batch = 1024;
void heap(benchmark::State& state) {
    std::array<void*, batch> blocks{};
    for (auto iteration : state) {
        for (auto& block : blocks) {
            block = ::operator new(bytes, std::align_val_t(bytes));
            benchmark::DoNotOptimize(block);
        }
        for (auto block : blocks) ::operator delete(block, std::align_val_t(bytes));
    }
    state.SetItemsProcessed(state.iterations() * batch);
}
void arena(benchmark::State& state) {
    memory::Arena arena(bytes * (batch + 1));
    for (auto iteration : state) {
        for (std::size_t i = 0; i < batch; ++i)
            benchmark::DoNotOptimize(arena.allocate(bytes, bytes).data);
        arena.reset();
    }
    state.SetItemsProcessed(state.iterations() * batch);
}
}
BENCHMARK(heap);
BENCHMARK(arena);
