// SPDX-License-Identifier: MIT OR Apache-2.0
#include <benchmark/benchmark.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "kloeden/buffer.hpp"
#include "kloeden/digital.hpp"

namespace fs = std::filesystem;

namespace {

fs::path fixtures_dir() {
    if (auto env = std::getenv("KLOEDEN_FIXTURES_DIR")) return fs::path(env);
    return fs::current_path() / ".." / "fixtures";
}

fs::path results_dir() {
    if (auto env = std::getenv("KLOEDEN_RESULTS_DIR")) return fs::path(env);
    return fs::current_path() / ".." / "results";
}

struct BenchContext {
    std::unique_ptr<kloeden::Fixture> fx;
    kloeden::GBM p{0.05, 0.2};
    BenchContext() { fx = std::make_unique<kloeden::Fixture>(kloeden::load_fixture(fixtures_dir())); }
};

BenchContext* g_ctx = nullptr;

void write_csv(const std::string& payoff, const kloeden::DeltaEstimate& est) {
    const auto& m = g_ctx->fx->meta();
    fs::create_directories(results_dir());
    auto out = results_dir() / ("cpp-strict_" + payoff + "_gbm_scalar.csv");
    std::ofstream f(out, std::ios::trunc);
    f << "impl,width,scheme,process,payoff,n_paths,n_steps,ns_per_path_step,paths_per_s,mc_mean,mc_stderr\n";
    f << "cpp-strict,scalar,euler,gbm," << payoff << ","
      << m.n_paths << "," << m.n_steps << ",-1,-1,"
      << est.mean << "," << est.stderr << "\n";
}

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    BenchContext ctx;
    g_ctx = &ctx;
    const auto& m = g_ctx->fx->meta();

    auto naive = kloeden::gbm_digital_naive_pathwise(
        g_ctx->fx->dw().data(), m.x0, m.dt, m.x0, m.n_paths, m.n_steps, g_ctx->p);
    write_csv("digital_naive", naive);

    auto bel = kloeden::gbm_digital_bel_lr(
        g_ctx->fx->dw().data(), m.x0, m.dt, m.x0, m.n_paths, m.n_steps, g_ctx->p);
    write_csv("digital_bel", bel);

    std::printf("cpp-strict digital_naive:  mean=%.6f stderr=%.6f\n", naive.mean, naive.stderr);
    std::printf("cpp-strict digital_bel:    mean=%.6f stderr=%.6f\n", bel.mean, bel.stderr);
    return 0;
}
