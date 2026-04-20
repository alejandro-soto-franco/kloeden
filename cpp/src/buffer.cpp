// SPDX-License-Identifier: MIT OR Apache-2.0
#include "kloeden/buffer.hpp"

#include <fcntl.h>
#include <openssl/sha.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace kloeden {

namespace {

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' || s.back() == '\r')) s.pop_back();
    std::size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(i);
}

std::string strip_quotes(std::string s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') return s.substr(1, s.size() - 2);
    return s;
}

// Minimal TOML-subset parser: one `key = value` per line, strings in quotes,
// numbers as-is. Enough for the flat Meta schema.
Meta parse_meta(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error(std::format("cannot open {}", path.string()));
    Meta m{};
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto key = trim(line.substr(0, eq));
        auto val = trim(line.substr(eq + 1));
        if (key == "seed") m.seed = std::stoull(val);
        else if (key == "n_paths") m.n_paths = std::stoull(val);
        else if (key == "n_steps") m.n_steps = std::stoull(val);
        else if (key == "dt") m.dt = std::stod(val);
        else if (key == "t") m.t = std::stod(val);
        else if (key == "x0") m.x0 = std::stod(val);
        else if (key == "rng_family") m.rng_family = strip_quotes(val);
        else if (key == "sha256") m.sha256 = strip_quotes(val);
    }
    return m;
}

std::string hex_sha256(const unsigned char* data, std::size_t len) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(data, len, digest);
    std::ostringstream oss;
    oss << std::hex;
    for (unsigned char b : digest) {
        oss << ((b < 0x10) ? "0" : "") << static_cast<int>(b);
    }
    return oss.str();
}

} // namespace

Fixture::Fixture(Meta meta, const double* ptr, std::size_t n_elems, int fd, void* mapped, std::size_t bytes)
    : meta_(std::move(meta)), ptr_(ptr), n_elems_(n_elems), fd_(fd), mapped_(mapped), bytes_(bytes) {}

Fixture::Fixture(Fixture&& other) noexcept
    : meta_(std::move(other.meta_)), ptr_(other.ptr_), n_elems_(other.n_elems_),
      fd_(other.fd_), mapped_(other.mapped_), bytes_(other.bytes_) {
    other.ptr_ = nullptr;
    other.n_elems_ = 0;
    other.fd_ = -1;
    other.mapped_ = nullptr;
    other.bytes_ = 0;
}

Fixture& Fixture::operator=(Fixture&& other) noexcept {
    if (this != &other) {
        this->~Fixture();
        new (this) Fixture(std::move(other));
    }
    return *this;
}

Fixture::~Fixture() {
    if (mapped_ && bytes_) munmap(mapped_, bytes_);
    if (fd_ >= 0) close(fd_);
}

Fixture load_fixture(const std::filesystem::path& dir) {
    auto bin_path = dir / "brownian_increments.bin";
    auto meta_path = dir / "brownian_increments.meta";

    auto meta = parse_meta(meta_path);

    int fd = ::open(bin_path.c_str(), O_RDONLY);
    if (fd < 0) throw std::runtime_error(std::format("open {}: {}", bin_path.string(), std::strerror(errno)));

    struct stat st{};
    if (fstat(fd, &st) != 0) {
        ::close(fd);
        throw std::runtime_error(std::format("fstat {}: {}", bin_path.string(), std::strerror(errno)));
    }

    std::size_t expected = meta.n_paths * meta.n_steps * sizeof(double);
    if (static_cast<std::size_t>(st.st_size) != expected) {
        ::close(fd);
        throw std::runtime_error(std::format("size mismatch: bin={}, expected={}", st.st_size, expected));
    }

    void* mapped = mmap(nullptr, expected, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        ::close(fd);
        throw std::runtime_error(std::format("mmap {}: {}", bin_path.string(), std::strerror(errno)));
    }

    auto computed = hex_sha256(static_cast<const unsigned char*>(mapped), expected);
    if (computed != meta.sha256) {
        munmap(mapped, expected);
        ::close(fd);
        throw std::runtime_error(std::format("sha256 mismatch: computed={}, meta={}", computed, meta.sha256));
    }

    std::size_t n_elems = meta.n_paths * meta.n_steps;
    return Fixture(std::move(meta), static_cast<const double*>(mapped), n_elems, fd, mapped, expected);
}

} // namespace kloeden
