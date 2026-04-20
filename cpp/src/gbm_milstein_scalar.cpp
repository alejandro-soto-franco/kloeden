// SPDX-License-Identifier: MIT OR Apache-2.0
#include "kloeden/schemes.hpp"

namespace kloeden {

void gbm_milstein_scalar(
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
    const double half_sigma_sq = 0.5 * sigma * sigma;
    for (std::size_t i = 0; i < n_paths; ++i) {
        double x = x0;
        const double* row = dw + i * n_steps;
        for (std::size_t s = 0; s < n_steps; ++s) {
            const double dw_s = row[s];
            // X_{n+1} = X_n + mu*X_n*dt + sigma*X_n*dW + 0.5*sigma^2*X_n*(dW^2 - dt)
            x += mu * x * dt + sigma * x * dw_s + half_sigma_sq * x * (dw_s * dw_s - dt);
        }
        terminal[i] = x;
    }
}

} // namespace kloeden
