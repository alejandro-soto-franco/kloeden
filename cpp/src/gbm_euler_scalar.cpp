// SPDX-License-Identifier: MIT OR Apache-2.0
#include "kloeden/schemes.hpp"

namespace kloeden {

void gbm_euler_scalar(
    const double* dw,
    double x0,
    double dt,
    std::size_t n_paths,
    std::size_t n_steps,
    const GBM& p,
    double* terminal)
{
    const double mu = p.mu;
    const double sigma = p.sigma;
    for (std::size_t i = 0; i < n_paths; ++i) {
        double x = x0;
        const double* row = dw + i * n_steps;
        for (std::size_t s = 0; s < n_steps; ++s) {
            // X_{n+1} = X_n + mu * X_n * dt + sigma * X_n * dW_n
            x += mu * x * dt + sigma * x * row[s];
        }
        terminal[i] = x;
    }
}

} // namespace kloeden
