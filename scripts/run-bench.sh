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

SCHEMES=(euler milstein taylor15)
CPP_TARGETS=(strict fast)
RUST_BENCHES=(pathwise_euler_gbm_scalar pathwise_milstein_gbm_scalar pathwise_taylor15_gbm_scalar)

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

# 3. Run C++ benchmarks for each (scheme, target).
for scheme in "${SCHEMES[@]}"; do
    for target in "${CPP_TARGETS[@]}"; do
        bin="$BUILD_DIR/bench/bench_gbm_${scheme}_scalar_${target}"
        log "step 3/5: cpp-${target} ${scheme} bench"
        KLOEDEN_FIXTURES_DIR="$FIXTURES_DIR" KLOEDEN_RESULTS_DIR="$RESULTS_DIR" \
            "${TASKSET[@]}" "$bin" --benchmark_format=console
    done
done

# 4. Run Rust benchmarks.
for bench_name in "${RUST_BENCHES[@]}"; do
    log "step 4/5: rust ${bench_name}"
    KLOEDEN_RESULTS_DIR="$RESULTS_DIR" \
        "${TASKSET[@]}" cargo bench --quiet --manifest-path rust/Cargo.toml -p kloeden-bench -- "$bench_name"
done

# 5. Collate + verify.
log "step 5/5: collate"
python3 scripts/collate.py

log "step 5/5: verify"
python3 scripts/verify.py

log "done"
