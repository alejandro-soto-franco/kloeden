// SPDX-License-Identifier: MIT OR Apache-2.0
#include "kloeden/schemes.hpp"

namespace kloeden {

void gbm_taylor15_scalar(
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
    const double sigma_sq = sigma * sigma;
    const double sigma_cu = sigma_sq * sigma;
    const double half_sigma_sq = 0.5 * sigma_sq;
    const double half_mu_sq_dt_sq = 0.5 * mu * mu * dt * dt;
    const double mu_sigma_dt = mu * sigma * dt;
    const double sigma_cu_over_6 = sigma_cu / 6.0;
    const double half_sigma_cu_dt = 0.5 * sigma_cu * dt;
    for (std::size_t i = 0; i < n_paths; ++i) {
        double x = x0;
        const double* row = dw + i * n_steps;
        for (std::size_t s = 0; s < n_steps; ++s) {
            const double dw_s = row[s];
            const double dw_sq = dw_s * dw_s;
            const double dw_cu = dw_sq * dw_s;
            x += mu * x * dt
               + sigma * x * dw_s
               + half_sigma_sq * x * (dw_sq - dt)
               + half_mu_sq_dt_sq * x
               + mu_sigma_dt * x * dw_s
               + sigma_cu_over_6 * x * dw_cu
               - half_sigma_cu_dt * x * dw_s;
        }
        terminal[i] = x;
    }
}

} // namespace kloeden
