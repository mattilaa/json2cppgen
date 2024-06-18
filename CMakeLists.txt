
cmake_minimum_required(VERSION 3.10)
project(json2cppgen)

# Create compile_commands.json in build directory
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include directory
include_directories(include)

include(FetchContent)

# Executable target
add_executable(json2cppgen src/main.cpp)
target_link_libraries(json2cppgen PRIVATE nlohmann_json::nlohmann_json)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2  # You can specify the version you need
)
FetchContent_MakeAvailable(json)

# Fetch GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add test executable
add_executable(
  test_json2cppgen
  tests/test.cpp
)

# Link test executable against gtest & gtest_main
target_link_libraries(
  test_json2cppgen gtest_main)

include(GoogleTest)
gtest_discover_tests(test_json2cppgen)

# Enable AddressSanitizer
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_FLAGS}")
endif()

# Fetch Google Benchmark
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.6.1.zip
)
FetchContent_MakeAvailable(googlebenchmark)

# Add benchmark executable
add_executable(
  benchmark_json2cppgen
  benchmarks/benchmark.cpp
)

# Link benchmark executable against benchmark library
target_link_libraries(
  benchmark_json2cppgen benchmark::benchmark
)

