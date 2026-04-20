#!/usr/bin/env python3
"""Cross-language integrity checks.

Table A (payoff=none): all impls' mc_mean must agree within 4 * (max stderr).
Table C (payoff starts with digital_):
  - digital_naive: mean must be exactly 0.
  - digital_bel: mean must be within 4*stderr of ANALYTIC_DELTA.

Exits 0 on pass, 1 on fail with a list of offending cells.
"""
from __future__ import annotations

import csv
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

TOL_STDERRS = 4.0
ANALYTIC_DELTA = 0.019724


def main() -> int:
    rows: list[dict[str, str]] = []
    for csv_path in sorted(RESULTS.glob("*.csv")):
        with csv_path.open() as f:
            rows.extend(csv.DictReader(f))

    if not rows:
        print("verify: no CSV rows in results/", file=sys.stderr)
        return 1

    fails: list[str] = []

    # Table A cross-impl check.
    by_cell_a: dict[tuple[str, str, str, str], list[dict[str, str]]] = defaultdict(list)
    for r in rows:
        if r["payoff"] != "none":
            continue
        key = (r["scheme"], r["process"], r["width"], r["payoff"])
        by_cell_a[key].append(r)

    for key, impls in sorted(by_cell_a.items()):
        if len(impls) < 2:
            continue
        means = [(r["impl"], float(r["mc_mean"]), float(r["mc_stderr"])) for r in impls]
        for i in range(len(means)):
            for j in range(i + 1, len(means)):
                a_impl, a_mean, a_se = means[i]
                b_impl, b_mean, b_se = means[j]
                tol = TOL_STDERRS * max(a_se, b_se)
                diff = abs(a_mean - b_mean)
                if diff > tol:
                    fails.append(
                        f"[A] {key} {a_impl} vs {b_impl}: |{a_mean} - {b_mean}| = {diff} > {tol}"
                    )

    # Table C per-row checks.
    table_c_rows = 0
    for r in rows:
        payoff = r["payoff"]
        if not payoff.startswith("digital_"):
            continue
        table_c_rows += 1
        impl = r["impl"]
        mean = float(r["mc_mean"])
        stderr = float(r["mc_stderr"])
        if payoff == "digital_naive":
            if mean != 0.0:
                fails.append(f"[C] {impl} digital_naive: mean={mean} expected 0")
        elif payoff == "digital_bel":
            diff = abs(mean - ANALYTIC_DELTA)
            tol = TOL_STDERRS * stderr
            if diff > tol:
                fails.append(
                    f"[C] {impl} digital_bel: |{mean} - {ANALYTIC_DELTA}| = {diff} > {tol} (stderr={stderr})"
                )

    if fails:
        print("verify: FAIL", file=sys.stderr)
        for f in fails:
            print("  " + f, file=sys.stderr)
        return 1

    n_cells_a = len(by_cell_a)
    n_rows_a = sum(len(v) for v in by_cell_a.values())
    print(f"verify: OK (Table A: {n_cells_a} cells, {n_rows_a} rows; Table C: {table_c_rows} rows)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
