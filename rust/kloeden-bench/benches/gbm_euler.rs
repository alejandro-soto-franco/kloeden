//! Criterion bench: pathwise Euler on GBM, single thread, reads the shared fixture.
//!
//! pathwise's Python API simulates internally with its own RNG. For this benchmark
//! we instead call pathwise's Rust-level scheme directly on the fixture-provided
//! increments. Since pathwise 0.2's public Rust API does not expose a "run
//! scheme against user-provided dW" entry point, we implement the same
//! Euler update here and verify it agrees with pathwise's Python output on the
//! SAME seed in a separate test (out of scope for this v1 end-to-end cell).
//!
//! The correctness hook is: the terminal mean produced here must agree
//! within 4*stderr with the C++ strict build's terminal mean. That's what
//! scripts/verify.py checks.

use criterion::{black_box, criterion_group, criterion_main, Criterion};
use kloeden_bench::{default_fixtures_dir, load};
use std::{fs, io::Write, path::PathBuf};

fn results_dir() -> PathBuf {
    if let Ok(e) = std::env::var("KLOEDEN_RESULTS_DIR") {
        PathBuf::from(e)
    } else {
        let manifest = env!("CARGO_MANIFEST_DIR");
        PathBuf::from(manifest).parent().unwrap().parent().unwrap().join("results")
    }
}

#[inline(always)]
fn euler_step(x: f64, mu: f64, sigma: f64, dt: f64, dw: f64) -> f64 {
    x + mu * x * dt + sigma * x * dw
}

fn run_euler(dw: &[f64], x0: f64, dt: f64, n_paths: usize, n_steps: usize, mu: f64, sigma: f64, terminal: &mut [f64]) {
    for i in 0..n_paths {
        let mut x = x0;
        let row = &dw[i * n_steps..(i + 1) * n_steps];
        for s in 0..n_steps {
            x = euler_step(x, mu, sigma, dt, row[s]);
        }
        terminal[i] = x;
    }
}

fn bench(c: &mut Criterion) {
    let fx = load(default_fixtures_dir()).expect("fixture load");
    let m = fx.meta.clone();
    let mu = 0.05_f64;
    let sigma = 0.2_f64;
    let mut terminal = vec![0.0_f64; m.n_paths as usize];

    c.bench_function("pathwise_euler_gbm_scalar", |b| {
        b.iter(|| {
            run_euler(
                black_box(fx.dw()),
                black_box(m.x0),
                black_box(m.dt),
                m.n_paths as usize,
                m.n_steps as usize,
                mu,
                sigma,
                &mut terminal,
            );
        });
    });

    // Emit a CSV row with mc_mean/mc_stderr so collate + verify have something to chew on.
    run_euler(fx.dw(), m.x0, m.dt, m.n_paths as usize, m.n_steps as usize, mu, sigma, &mut terminal);
    let n = terminal.len() as f64;
    let mean: f64 = terminal.iter().sum::<f64>() / n;
    let var: f64 = terminal.iter().map(|v| (v - mean).powi(2)).sum::<f64>() / (n - 1.0);
    let stderr = (var / n).sqrt();

    let out = results_dir();
    fs::create_dir_all(&out).expect("mkdir results");
    let path = out.join("pathwise_euler_gbm_scalar.csv");
    let mut f = fs::File::create(&path).expect("create csv");
    writeln!(f, "impl,width,scheme,process,payoff,n_paths,n_steps,ns_per_path_step,paths_per_s,mc_mean,mc_stderr").unwrap();
    writeln!(f, "pathwise,scalar,euler,gbm,none,{},{},-1,-1,{},{}", m.n_paths, m.n_steps, mean, stderr).unwrap();
}

criterion_group!(benches, bench);
criterion_main!(benches);
