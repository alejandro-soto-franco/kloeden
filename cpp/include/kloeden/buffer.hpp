// SPDX-License-Identifier: MIT OR Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>

namespace kloeden {

struct Meta {
    std::uint64_t seed;
    std::uint64_t n_paths;
    std::uint64_t n_steps;
    double dt;
    double t;
    double x0;
    std::string rng_family;
    std::string sha256;
};

class Fixture {
public:
    Fixture(Meta meta, const double* ptr, std::size_t n_elems, int fd, void* mapped, std::size_t bytes);
    ~Fixture();

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&& other) noexcept;
    Fixture& operator=(Fixture&& other) noexcept;

    const Meta& meta() const noexcept { return meta_; }
    std::span<const double> dw() const noexcept { return {ptr_, n_elems_}; }

private:
    Meta meta_;
    const double* ptr_;
    std::size_t n_elems_;
    int fd_;
    void* mapped_;
    std::size_t bytes_;
};

// Loads fixtures/brownian_increments.{bin,meta} from the given directory,
// verifies sha256, and returns a Fixture owning the mmap.
// Throws std::runtime_error on any mismatch.
Fixture load_fixture(const std::filesystem::path& dir);

} // namespace kloeden
