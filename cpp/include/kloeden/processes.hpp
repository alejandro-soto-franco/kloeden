// SPDX-License-Identifier: MIT OR Apache-2.0
#pragma once

namespace kloeden {

// Geometric Brownian motion: dX = mu * X dt + sigma * X dW.
struct GBM {
    double mu;
    double sigma;
};

} // namespace kloeden
