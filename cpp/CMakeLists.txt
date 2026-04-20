cmake_minimum_required(VERSION 3.20)
project(kloeden VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(benchmark REQUIRED)
find_package(GTest REQUIRED)
include(CTest)

set(KLOEDEN_SOURCES
    src/placeholder.cpp
)

# Strict IEEE build target
add_library(kloeden_strict STATIC ${KLOEDEN_SOURCES})
target_include_directories(kloeden_strict PUBLIC include)
target_compile_options(kloeden_strict PRIVATE
    -O3 -march=native -Wall -Wextra -Wpedantic
)

# Fastmath build target (throughput-only; correctness uses strict)
add_library(kloeden_fast STATIC ${KLOEDEN_SOURCES})
target_include_directories(kloeden_fast PUBLIC include)
target_compile_options(kloeden_fast PRIVATE
    -O3 -march=native -ffast-math -Wall -Wextra -Wpedantic
)

# Subdirs (added as Task 10+)
# add_subdirectory(bench)
# add_subdirectory(tests)
