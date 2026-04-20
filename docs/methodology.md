# Methodology

This file is populated in a follow-on plan (milestone 5+ of the spec). For now, the v1 scaffold covers scalar Euler on GBM only; the full methodology prose arrives once Table A is populated.

## Controls applied today

- Single thread. No rayon. No OMP. No threading at any layer.
- Pinned core via `taskset -c 2` when `run-bench.sh` orchestrates.
- Fixed seed (`20260420`) on the Brownian-increment buffer.
- `sha256` of the buffer pinned in `fixtures/brownian_increments.meta` and checked on every load.
