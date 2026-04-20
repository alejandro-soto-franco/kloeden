// SPDX-License-Identifier: MIT OR Apache-2.0
#include "kloeden/digital.hpp"

#include <algorithm>
#include <cmath>

namespace kloeden {

DeltaEstimate gbm_digital_bel_lr(
    const double* dw,
    double x0,
    double dt,
    double strike,
    std::size_t n_paths,
    std::size_t n_steps,
    const GBM& p)
{
    const double mu = p.mu;
    const double sigma = p.sigma;
    const double horizon = dt * static_cast<double>(n_steps);
    const double bel_scale = 1.0 / (horizon * sigma * x0);

    double sum = 0.0;
    double sum_sq = 0.0;
    for (std::size_t i = 0; i < n_paths; ++i) {
        double x = x0;
        double w_total = 0.0;
        const double* row = dw + i * n_steps;
        for (std::size_t s = 0; s < n_steps; ++s) {
            const double dw_s = row[s];
            w_total += dw_s;
            x += mu * x * dt + sigma * x * dw_s;
        }
        const double f = (x > strike) ? 1.0 : 0.0;
        const double delta_sample = f * w_total * bel_scale;
        sum += delta_sample;
        sum_sq += delta_sample * delta_sample;
    }
    const double n = static_cast<double>(n_paths);
    const double mean = sum / n;
    const double var = (sum_sq / n) - mean * mean;
    const double stderr_mean = std::sqrt(std::max(0.0, var) / n);
    return DeltaEstimate{mean, stderr_mean, n_paths};
}

} // namespace kloeden
