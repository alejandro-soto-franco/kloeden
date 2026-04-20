// SPDX-License-Identifier: MIT OR Apache-2.0
#include "kloeden/digital.hpp"

#include <algorithm>
#include <cmath>

namespace kloeden {

DeltaEstimate gbm_digital_naive_pathwise(
    const double* dw,
    double x0,
    double dt,
    double /*strike*/,
    std::size_t n_paths,
    std::size_t n_steps,
    const GBM& p)
{
    const double mu = p.mu;
    const double sigma = p.sigma;
    double sum = 0.0;
    double sum_sq = 0.0;
    for (std::size_t i = 0; i < n_paths; ++i) {
        double x = x0;
        const double* row = dw + i * n_steps;
        for (std::size_t s = 0; s < n_steps; ++s) {
            x += mu * x * dt + sigma * x * row[s];
        }
        // Naive pathwise diff for digital: f'(X_T) * dX_T/dS_0.
        // For f = 1{x > K}, f'(x) = 0 almost everywhere. The Dirac spike
        // at x = K has measure zero, so MC samples never see it.
        const double f_prime = 0.0;
        const double dX_dS0 = x / x0;
        const double delta_sample = f_prime * dX_dS0;
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
