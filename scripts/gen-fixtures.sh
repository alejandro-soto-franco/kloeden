#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "gen-fixtures: not yet implemented (stub)"
echo "will call: cargo run --release -p kloeden-buffer-gen -- --seed 20260420 --n-paths 10000 --n-steps 256"
exit 1
