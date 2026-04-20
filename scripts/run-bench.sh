#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
RESULTS_DIR="${RESULTS_DIR:-$ROOT/results}"
FIXTURES_DIR="${FIXTURES_DIR:-$ROOT/fixtures}"
CORE="${CORE:-2}"

log() { printf '[run-bench] %s\n' "$*"; }

if ! command -v taskset >/dev/null 2>&1; then
    log "taskset not found; running without core pinning"
    TASKSET=(env)
else
    TASKSET=(taskset -c "$CORE")
fi

# 1. Generate fixtures.
log "step 1/5: gen-fixtures"
./scripts/gen-fixtures.sh

# 2. Build C++ if needed.
if [[ ! -d "$BUILD_DIR" ]]; then
    log "step 2/5: cmake configure"
    cmake -S cpp -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi
log "step 2/5: cmake build"
cmake --build "$BUILD_DIR" -j

# 3. Run C++ benchmarks (strict + fast).
log "step 3/5: cpp-strict bench"
KLOEDEN_FIXTURES_DIR="$FIXTURES_DIR" KLOEDEN_RESULTS_DIR="$RESULTS_DIR" \
    "${TASKSET[@]}" "$BUILD_DIR/bench/bench_gbm_euler_scalar_strict" --benchmark_format=console
log "step 3/5: cpp-fast bench"
KLOEDEN_FIXTURES_DIR="$FIXTURES_DIR" KLOEDEN_RESULTS_DIR="$RESULTS_DIR" \
    "${TASKSET[@]}" "$BUILD_DIR/bench/bench_gbm_euler_scalar_fast" --benchmark_format=console

# 4. Run Rust bench.
log "step 4/5: rust bench"
KLOEDEN_RESULTS_DIR="$RESULTS_DIR" \
    "${TASKSET[@]}" cargo bench --quiet --manifest-path rust/Cargo.toml -p kloeden-bench -- pathwise_euler_gbm_scalar

# 5. Collate + verify.
log "step 5/5: collate"
python3 scripts/collate.py

log "step 5/5: verify"
python3 scripts/verify.py

log "done"
