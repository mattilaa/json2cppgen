
#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>

// Example function to benchmark
void ExampleFunction(std::vector<int>& data)
{{
    std::sort(data.begin(), data.end());
}}

// Benchmark for the example function
static void BM_ExampleFunction(benchmark::State& state)
{{
    // Setup code
    std::vector<int> data(state.range(0));
    std::generate(data.begin(), data.end(), std::rand);

    // Run the benchmark
    for(auto _ : state)
    {{
        // We need to make a copy of the data for each iteration
        std::vector<int> data_copy = data;
        ExampleFunction(data_copy);
    }}

    // Set the items processed per iteration
    state.SetItemsProcessed(state.iterations() * state.range(0));
}}

// Register the function as a benchmark
BENCHMARK(BM_ExampleFunction)->Range(8, 8<<10);

BENCHMARK_MAIN();
        