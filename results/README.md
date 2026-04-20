# kloeden benchmark results

This directory holds the raw CSV rows produced by `scripts/run-bench.sh` (gitignored) and the documented methodology for the host machine.

## Host

Populated by `scripts/run-bench.sh` on every run (prints gcc, clang, rustc, kernel, CPU governor, turbo state, host CPU flags).

## Methodology

See [`../docs/methodology.md`](../docs/methodology.md).

## Reading a CSV row

Columns (see `collate.py` for source of truth):

```
impl,width,scheme,process,payoff,n_paths,n_steps,ns_per_path_step,paths_per_s,mc_mean,mc_stderr
```

One row per `(impl, width, scheme, process, payoff)` cell per run.
