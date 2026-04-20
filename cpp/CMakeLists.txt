cmake_minimum_required(VERSION 3.20)
project(kloeden VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(benchmark REQUIRED)
find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)
include(CTest)

set(KLOEDEN_SOURCES
    src/buffer.cpp
    src/gbm_euler_scalar.cpp
    src/gbm_milstein_scalar.cpp
    src/placeholder.cpp
)

# Strict IEEE build target
add_library(kloeden_strict STATIC ${KLOEDEN_SOURCES})
target_include_directories(kloeden_strict PUBLIC include)
target_compile_options(kloeden_strict PRIVATE
    -O3 -march=native -Wall -Wextra -Wpedantic
)
target_link_libraries(kloeden_strict PUBLIC OpenSSL::Crypto)

# Fastmath build target (throughput-only; correctness uses strict)
add_library(kloeden_fast STATIC ${KLOEDEN_SOURCES})
target_include_directories(kloeden_fast PUBLIC include)
target_compile_options(kloeden_fast PRIVATE
    -O3 -march=native -ffast-math -Wall -Wextra -Wpedantic
)
target_link_libraries(kloeden_fast PUBLIC OpenSSL::Crypto)

enable_testing()
add_subdirectory(tests)
add_subdirectory(bench)
