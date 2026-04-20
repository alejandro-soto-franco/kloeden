// SPDX-License-Identifier: MIT OR Apache-2.0
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

#include "kloeden/buffer.hpp"
#include "kloeden/schemes.hpp"

namespace {

std::filesystem::path fixtures_dir() {
    if (auto env = std::getenv("KLOEDEN_FIXTURES_DIR")) return std::filesystem::path(env);
    return std::filesystem::current_path() / ".." / "fixtures";
}

} // namespace

TEST(GbmMilsteinScalar, TerminalMeanMatchesAnalytic) {
    auto fx = kloeden::load_fixture(fixtures_dir());
    const auto& m = fx.meta();

    kloeden::GBM p{.mu = 0.05, .sigma = 0.2};
    std::vector<double> terminal(m.n_paths);
    kloeden::gbm_milstein_scalar(
        fx.dw().data(), m.x0, m.dt, m.n_paths, m.n_steps, p, terminal.data());

    double sum = 0.0;
    for (double v : terminal) sum += v;
    double mean = sum / static_cast<double>(m.n_paths);
    double sq = 0.0;
    for (double v : terminal) { double d = v - mean; sq += d * d; }
    double sample_var = sq / static_cast<double>(m.n_paths - 1);
    double stderr_mean = std::sqrt(sample_var / static_cast<double>(m.n_paths));

    double analytic = m.x0 * std::exp(p.mu * m.t);
    double diff = std::fabs(mean - analytic);
    EXPECT_LE(diff, 4.0 * stderr_mean)
        << "mean=" << mean << " analytic=" << analytic << " stderr=" << stderr_mean;
}
