# Memory Allocator and Inspector

A C++20 bump arena and fixed-size free-list pool with an interactive Dear ImGui inspector for usage and allocation lifetimes.

## Build and run

Requires CMake 3.24+, a C++20 compiler and the platform's SDL build prerequisites. CMake downloads pinned releases for SDL3, Dear ImGui, GoogleTest and Google Benchmark.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
./build/inspector
./build/memory_bench --benchmark_repetitions=5
```

With a multi-config generator, look in `build/Release/`. In the inspector, allocate a few arena blocks with different alignment settings and reset the arena. Switch to the pool, allocate slots, then release the oldest slot. The table shows elapsed lifetimes while the matching allocation event remains in history.

## Design and reproducibility
