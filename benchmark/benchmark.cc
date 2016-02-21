#include <benchmark/benchmark.h>

#include "scientist.hh"

int function()
{
    return 1;
}

static void WithScientist(benchmark::State& state)
{
    while (state.KeepRunning())
    {
        int res = Scientist<int>::Science("benchmark", [](ExperimentInterface<int>& e)
        {
            e.Use(function);
            e.Try(function);
        });
    }
}

static void WithoutScientist(benchmark::State& state)
{
    while(state.KeepRunning())
        int res = function();
}

BENCHMARK(WithScientist);
BENCHMARK(WithoutScientist);

BENCHMARK_MAIN();
