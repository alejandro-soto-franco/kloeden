// SPDX-License-Identifier: MIT OR Apache-2.0
#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "kloeden/buffer.hpp"
#include "kloeden/schemes.hpp"

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

#ifndef KLOEDEN_IMPL_NAME
#define KLOEDEN_IMPL_NAME "cpp-strict"
#endif

struct BenchContext {
    std::unique_ptr<kloeden::Fixture> fx;
    std::vector<double> terminal;
    kloeden::GBM p{0.05, 0.2};

    BenchContext() {
        fx = std::make_unique<kloeden::Fixture>(kloeden::load_fixture(fixtures_dir()));
        terminal.resize(fx->meta().n_paths);
    }
};

BenchContext* g_ctx = nullptr;

void BM_gbm_taylor15_scalar(benchmark::State& state) {
    const auto& m = g_ctx->fx->meta();
    for (auto _ : state) {
        kloeden::gbm_taylor15_scalar(
            g_ctx->fx->dw().data(), m.x0, m.dt, m.n_paths, m.n_steps, g_ctx->p, g_ctx->terminal.data());
        benchmark::DoNotOptimize(g_ctx->terminal.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(m.n_paths * m.n_steps));
}
BENCHMARK(BM_gbm_taylor15_scalar)->Unit(benchmark::kMillisecond)->MinWarmUpTime(0.1)->MinTime(1.0);

void emit_csv() {
    const auto& m = g_ctx->fx->meta();

    kloeden::gbm_taylor15_scalar(
        g_ctx->fx->dw().data(), m.x0, m.dt, m.n_paths, m.n_steps, g_ctx->p, g_ctx->terminal.data());
    double sum = 0.0;
    for (double v : g_ctx->terminal) sum += v;
    double mean = sum / static_cast<double>(m.n_paths);
    double sq = 0.0;
    for (double v : g_ctx->terminal) { double d = v - mean; sq += d*d; }
    double var = sq / static_cast<double>(m.n_paths - 1);
    double stderr_mean = std::sqrt(var / static_cast<double>(m.n_paths));

    constexpr int reps = 20;
    std::vector<double> ns_per_run(reps);
    for (int r = 0; r < reps; ++r) {
        auto t0 = std::chrono::steady_clock::now();
        kloeden::gbm_taylor15_scalar(
            g_ctx->fx->dw().data(), m.x0, m.dt, m.n_paths, m.n_steps, g_ctx->p, g_ctx->terminal.data());
        auto t1 = std::chrono::steady_clock::now();
        ns_per_run[r] = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    std::sort(ns_per_run.begin(), ns_per_run.end());
    double median_ns = ns_per_run[reps / 2];
    double path_steps = static_cast<double>(m.n_paths) * static_cast<double>(m.n_steps);
    double ns_per_path_step = median_ns / path_steps;
    double path_steps_per_s = path_steps / (median_ns * 1e-9);

    fs::create_directories(results_dir());
    auto out = results_dir() / (std::string(KLOEDEN_IMPL_NAME) + "_taylor15_gbm_scalar.csv");
    std::ofstream f(out, std::ios::trunc);
    f << "impl,width,scheme,process,payoff,n_paths,n_steps,ns_per_path_step,paths_per_s,mc_mean,mc_stderr\n";
    f << KLOEDEN_IMPL_NAME << ",scalar,taylor15,gbm,none,"
      << m.n_paths << "," << m.n_steps << ","
      << ns_per_path_step << "," << path_steps_per_s << ","
      << mean << "," << stderr_mean << "\n";
}

} // namespace

int main(int argc, char** argv) {
    BenchContext ctx;
    g_ctx = &ctx;

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    emit_csv();
    return 0;
}
