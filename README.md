# kloeden

Benchmark companion to [`elworthy`](https://github.com/alejandro-soto-franco/elworthy) and [`pathwise`](https://github.com/alejandro-soto-franco/pathwise). Hand-written SIMD C++ benchmarked against Rust with LLVM and Cranelift backends on SDE + Monte Carlo Greeks workloads, single thread, pinned core.

Named after Peter Kloeden (Kloeden-Platen, *Numerical Solution of Stochastic Differential Equations*, Springer 1992).

## Status

v1 scaffold: scalar Euler on GBM wired end-to-end for both sides. Milstein, Taylor 1.5, AVX2 F64X4, and the Bismut-Elworthy-Li digital-delta correctness table are follow-on work.

## Quick start

Prereqs (Fedora 43):

```bash
sudo dnf install -y gtest-devel google-benchmark-devel openssl-devel cmake gcc-c++ python3
```

Rust toolchain: whichever `rustup` default is active (MSRV 1.75).

End-to-end:

```bash
./scripts/run-bench.sh
cat docs/results.md
```

What the orchestrator does (see `scripts/run-bench.sh`):

1. `gen-fixtures.sh` produces deterministic `fixtures/brownian_increments.{bin,meta}` (sha256 pinned).
2. CMake configure + build (`kloeden_strict`, `kloeden_fast`).
3. Run Google Benchmark for both C++ targets on the shared buffer.
4. Run Criterion for the Rust side on the same buffer.
5. `collate.py` merges CSVs into `docs/results.md`; `verify.py` asserts cross-language integrity within 4 sigma.

Outputs:

- `results/*.csv`: one row per (impl, scheme, process, width, payoff) cell. Gitignored.
- `docs/results.md`: human-readable Table A (more tables land in follow-on plans).
- `fixtures/*`: Brownian increments + TOML sidecar. Gitignored.

## SELinux note (Fedora / RHEL)

Cranelift JIT (used inside `elworthy-rt`, which kloeden will add to the benchmark mix in the next plan) needs `execmem`. Under SELinux `enforcing` you may hit:

```
Error: cranelift module error: Backend error: unable to make memory readable+executable
```

Workarounds: run via `cargo test`, or `sudo setsebool -P selinuxuser_execheap 1`. The current scaffold plan does not exercise Cranelift; this note is pre-positioned for the follow-on.

## License

Dual-licensed under MIT OR Apache-2.0.
