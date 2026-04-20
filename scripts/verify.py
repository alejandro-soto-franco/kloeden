#!/usr/bin/env python3
"""Cross-language integrity: every (scheme, process, width, payoff) cell must
have every impl's mc_mean within 4 * (max mc_stderr) of every other impl's mc_mean.

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


def main() -> int:
    rows: list[dict[str, str]] = []
    for csv_path in sorted(RESULTS.glob("*.csv")):
        with csv_path.open() as f:
            rows.extend(csv.DictReader(f))

    if not rows:
        print("verify: no CSV rows in results/", file=sys.stderr)
        return 1

    by_cell: dict[tuple[str, str, str, str], list[dict[str, str]]] = defaultdict(list)
    for r in rows:
        key = (r["scheme"], r["process"], r["width"], r["payoff"])
        by_cell[key].append(r)

    fails: list[str] = []
    for key, impls in sorted(by_cell.items()):
        if len(impls) < 2:
            continue  # nothing to compare
        means = [(r["impl"], float(r["mc_mean"]), float(r["mc_stderr"])) for r in impls]
        for i in range(len(means)):
            for j in range(i + 1, len(means)):
                a_impl, a_mean, a_se = means[i]
                b_impl, b_mean, b_se = means[j]
                tol = TOL_STDERRS * max(a_se, b_se)
                diff = abs(a_mean - b_mean)
                if diff > tol:
                    fails.append(
                        f"{key} {a_impl} vs {b_impl}: |{a_mean} - {b_mean}| = {diff} > {tol}"
                    )

    if fails:
        print("verify: FAIL", file=sys.stderr)
        for f in fails:
            print("  " + f, file=sys.stderr)
        return 1

    print(f"verify: OK ({len(by_cell)} cells, {len(rows)} rows)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
