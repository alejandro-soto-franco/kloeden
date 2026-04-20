// SPDX-License-Identifier: MIT OR Apache-2.0
#pragma once

#include <cstddef>

#include "kloeden/processes.hpp"

namespace kloeden {

// Euler-Maruyama on GBM, scalar (one path at a time).
//
// Inputs:
//   dw        : Brownian increments, length n_paths * n_steps, row-major by path.
//   x0, dt    : initial price and step size (dt must match what generated dw).
//   n_paths, n_steps: fixture dimensions.
//   p         : GBM parameters.
// Output:
//   terminal  : length n_paths, written in order.
void gbm_euler_scalar(
    const double* dw,
    double x0,
    double dt,
    std::size_t n_paths,
    std::size_t n_steps,
    const GBM& p,
    double* terminal);

// Milstein on GBM, scalar (one path at a time).
// Strong order 1. Same signature as gbm_euler_scalar; uses same dW buffer.
//
//   X_{n+1} = X_n + mu*X_n*dt + sigma*X_n*dW_n + 0.5*sigma^2*X_n*(dW_n^2 - dt)
void gbm_milstein_scalar(
    const double* dw,
    double x0,
    double dt,
    std::size_t n_paths,
    std::size_t n_steps,
    const GBM& p,
    double* terminal);

} // namespace kloeden
