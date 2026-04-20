# kloeden

Benchmark companion to [`elworthy`](https://github.com/alejandro-soto-franco/elworthy) and [`pathwise`](https://github.com/alejandro-soto-franco/pathwise). Hand-written SIMD C++ benchmarked against Rust with LLVM and Cranelift backends on SDE + Monte Carlo Greeks workloads, single thread, pinned core.

Named after Peter Kloeden (Kloeden-Platen, *Numerical Solution of Stochastic Differential Equations*, Springer 1992).

## Status

v1 scaffold: scalar Euler on GBM wired end-to-end for both sides. Milstein, Taylor 1.5, AVX2 F64X4, and the Bismut-Elworthy-Li digital-delta correctness table are follow-on work.

## Quick start

```bash
sudo dnf install -y gtest-devel google-benchmark-devel cmake gcc-c++
./scripts/gen-fixtures.sh
./scripts/run-bench.sh
cat docs/results.md
```

## License

Dual-licensed under MIT OR Apache-2.0.
