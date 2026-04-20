#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "run-bench: not yet implemented (stub)"
echo "will orchestrate: gen-fixtures -> cpp bench -> rust bench -> collate -> verify"
exit 1
