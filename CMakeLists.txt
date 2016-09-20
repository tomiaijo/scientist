cmake_minimum_required(VERSION 2.8)
project(Scientist)

include_directories("include/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Tests
set(BUILD_GTEST ON)
add_subdirectory(test/googletest)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(TEST_SOURCES test/ignore.cc test/publish.cc test/experiment.cc test/compare.cc test/run_if.cc test/cleanup.cc test/context.cc test/before_run.cc test/multiple_candidates.cc)

add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests gtest gtest_main)

add_test(all_tests tests)

# Benchmarks
add_subdirectory(benchmark/benchmark)
include_directories(benchmark/benchmark/include)
add_executable(benchmarks benchmark/benchmark.cc)
target_link_libraries(benchmarks benchmark)
