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
