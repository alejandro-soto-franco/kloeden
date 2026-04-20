#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

SEED="${SEED:-20260420}"
N_PATHS="${N_PATHS:-10000}"
N_STEPS="${N_STEPS:-256}"
T="${T:-1.0}"
X0="${X0:-100.0}"

echo "[gen-fixtures] seed=$SEED n_paths=$N_PATHS n_steps=$N_STEPS t=$T x0=$X0"

cargo run --quiet --release \
    --manifest-path rust/Cargo.toml \
    -p kloeden-buffer-gen -- \
    --seed "$SEED" \
    --n-paths "$N_PATHS" \
    --n-steps "$N_STEPS" \
    --t "$T" \
    --x0 "$X0" \
    --out-dir fixtures

BIN=fixtures/brownian_increments.bin
META=fixtures/brownian_increments.meta

if [[ ! -f "$BIN" || ! -f "$META" ]]; then
    echo "[gen-fixtures] missing outputs" >&2
    exit 1
fi

COMPUTED=$(sha256sum "$BIN" | awk '{print $1}')
DECLARED=$(awk -F' = ' '/^sha256/ {gsub(/"/, "", $2); print $2}' "$META")

if [[ "$COMPUTED" != "$DECLARED" ]]; then
    echo "[gen-fixtures] sha256 mismatch" >&2
    echo "  computed: $COMPUTED" >&2
    echo "  declared: $DECLARED" >&2
    exit 1
fi

echo "[gen-fixtures] OK (sha256=$COMPUTED)"
