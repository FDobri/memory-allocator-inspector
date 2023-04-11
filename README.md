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

The arena uses `std::align` and retains storage across resets. The pool uses aligned, fixed-stride slots and checks allocation IDs to reject stale or duplicate releases. Both allocators are single-threaded. The demo records the latest 256 events per allocator in a ring. Callers own any object construction and destruction in the returned raw storage.

The benchmark reserves external state before the timed loop. Each iteration allocates and releases a batch of 1,024 aligned 64-byte blocks. Arena reset releases the batch as a unit. Google Benchmark reports time per batch and throughput. Record CPU, OS, compiler, build configuration and repetition settings when sharing results.

For sanitizer validation, configure a separate build with `-DINSPECTOR_ASAN=ON -DINSPECTOR_GUI=OFF -DINSPECTOR_BENCH=OFF`, then build and run CTest using RelWithDebInfo. On MSVC, run from a Visual Studio Developer PowerShell so the ASan runtime is on PATH. ASan covers accesses to the backing allocation. Pool release validation uses allocation IDs.
