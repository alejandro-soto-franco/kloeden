// SPDX-License-Identifier: MIT OR Apache-2.0
#pragma once

#include <cstddef>

#include "kloeden/processes.hpp"

namespace kloeden {

struct DeltaEstimate {
    double mean;
    double stderr;
    std::size_t n_paths;
};

// Naive pathwise differentiation of a digital payoff on GBM.
// Per path: delta_sample = f'(X_T) * dX_T/dS_0 where f is 1{X_T > K}.
// Since f' = 0 almost everywhere (Dirac term excluded), the kernel returns
// 0 for every path. Biased; here to document the silent failure mode.
DeltaEstimate gbm_digital_naive_pathwise(
    const double* dw,
    double x0,
    double dt,
    double strike,
    std::size_t n_paths,
    std::size_t n_steps,
    const GBM& p);

// Hand-rolled Bismut-Elworthy-Li constant-flow weight for GBM digital.
// Per path: delta_sample = f(X_T) * W_T / (T * sigma * S_0)
// where W_T = sum_n dW_n is the terminal Brownian value.
// Unbiased for any bounded payoff including digitals.
DeltaEstimate gbm_digital_bel_lr(
    const double* dw,
    double x0,
    double dt,
    double strike,
    std::size_t n_paths,
    std::size_t n_steps,
    const GBM& p);

} // namespace kloeden
