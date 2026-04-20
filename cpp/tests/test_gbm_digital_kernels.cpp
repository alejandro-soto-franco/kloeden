// SPDX-License-Identifier: MIT OR Apache-2.0
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>

#include "kloeden/buffer.hpp"
#include "kloeden/digital.hpp"

namespace {

std::filesystem::path fixtures_dir() {
    if (auto env = std::getenv("KLOEDEN_FIXTURES_DIR")) return std::filesystem::path(env);
    return std::filesystem::current_path() / ".." / "fixtures";
}

// Analytic digital-delta for GBM, K = S_0, mu=0.05, sigma=0.2, T=1:
// d_2 = (mu - sigma^2/2) * sqrt(T) / sigma = 0.15
// n(d_2) = 0.39447
// Delta = n(d_2) / (S_0 * sigma * sqrt(T)) = 0.019724
constexpr double ANALYTIC_DELTA = 0.019724;

} // namespace

TEST(GbmDigitalNaivePathwise, ReturnsZero) {
    auto fx = kloeden::load_fixture(fixtures_dir());
    const auto& m = fx.meta();
    kloeden::GBM p{.mu = 0.05, .sigma = 0.2};
    auto est = kloeden::gbm_digital_naive_pathwise(
        fx.dw().data(), m.x0, m.dt, m.x0, m.n_paths, m.n_steps, p);
    EXPECT_EQ(est.mean, 0.0) << "naive pathwise must literally return 0 per path";
    EXPECT_EQ(est.stderr, 0.0);
}

TEST(GbmDigitalBelLr, MatchesAnalyticWithin4Sigma) {
    auto fx = kloeden::load_fixture(fixtures_dir());
    const auto& m = fx.meta();
    kloeden::GBM p{.mu = 0.05, .sigma = 0.2};
    auto est = kloeden::gbm_digital_bel_lr(
        fx.dw().data(), m.x0, m.dt, m.x0, m.n_paths, m.n_steps, p);
    const double diff = std::fabs(est.mean - ANALYTIC_DELTA);
    EXPECT_LE(diff, 4.0 * est.stderr)
        << "mean=" << est.mean << " analytic=" << ANALYTIC_DELTA
        << " stderr=" << est.stderr;
}
