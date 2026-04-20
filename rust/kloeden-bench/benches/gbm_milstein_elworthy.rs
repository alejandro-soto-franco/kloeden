//! Criterion bench: elworthy Cranelift-JIT Milstein on GBM.
//! No cached variant at elworthy-rt 0.1.1; recompile cost (~400us for 4
//! kernels) is ~7% of MC cost at 10k paths x 256 steps. Accept the noise.

use criterion::{black_box, criterion_group, criterion_main, Criterion};
use elworthy_expr::Expr;
use elworthy_rt::milstein_scalar_jit;
use kloeden_bench::{default_fixtures_dir, load};
use std::{fs, io::Write, path::PathBuf, time::Instant};

fn results_dir() -> PathBuf {
    if let Ok(e) = std::env::var("KLOEDEN_RESULTS_DIR") {
        PathBuf::from(e)
    } else {
        let manifest = env!("CARGO_MANIFEST_DIR");
        PathBuf::from(manifest).parent().unwrap().parent().unwrap().join("results")
    }
}

fn bench(c: &mut Criterion) {
    let fx = load(default_fixtures_dir()).expect("fixture load");
    let m = fx.meta.clone();

    let mu_expr = Expr::c(0.05) * Expr::state(0);
    let sigma_expr = Expr::c(0.2) * Expr::state(0);
    let payoff_expr = Expr::state(0);

    // Warm.
    milstein_scalar_jit(
        &mu_expr, &sigma_expr, &payoff_expr, &[],
        m.x0, m.t, m.n_steps as usize, m.n_paths as usize, m.seed,
    ).expect("warm");

    c.bench_function("elworthy_milstein_gbm_scalar", |b| {
        b.iter(|| {
            let est = milstein_scalar_jit(
                black_box(&mu_expr), black_box(&sigma_expr), black_box(&payoff_expr),
                black_box(&[]),
                black_box(m.x0), black_box(m.t),
                m.n_steps as usize, m.n_paths as usize, m.seed,
            ).expect("jit");
            black_box(est);
        });
    });

    let est = milstein_scalar_jit(
        &mu_expr, &sigma_expr, &payoff_expr, &[],
        m.x0, m.t, m.n_steps as usize, m.n_paths as usize, m.seed,
    ).expect("jit");

    let reps = 20usize;
    let mut ns_per_run: Vec<f64> = Vec::with_capacity(reps);
    for _ in 0..reps {
        let t0 = Instant::now();
        let _ = milstein_scalar_jit(
            &mu_expr, &sigma_expr, &payoff_expr, &[],
            m.x0, m.t, m.n_steps as usize, m.n_paths as usize, m.seed,
        ).expect("jit");
        ns_per_run.push(t0.elapsed().as_nanos() as f64);
    }
    ns_per_run.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let median_ns = ns_per_run[reps / 2];
    let path_steps = (m.n_paths * m.n_steps) as f64;
    let ns_per_path_step = median_ns / path_steps;
    let path_steps_per_s = path_steps / (median_ns * 1e-9);

    let out = results_dir();
    fs::create_dir_all(&out).expect("mkdir results");
    let path = out.join("elworthy_milstein_gbm_scalar.csv");
    let mut f = fs::File::create(&path).expect("create csv");
    writeln!(f, "impl,width,scheme,process,payoff,n_paths,n_steps,ns_per_path_step,paths_per_s,mc_mean,mc_stderr").unwrap();
    writeln!(f, "elworthy,scalar,milstein,gbm,none,{},{},{},{},{},{}",
        m.n_paths, m.n_steps, ns_per_path_step, path_steps_per_s, est.mean, est.stderr).unwrap();
}

criterion_group!(benches, bench);
criterion_main!(benches);
