//! Criterion bench: scalar Milstein on GBM, single thread, reads the shared fixture.

use criterion::{black_box, criterion_group, criterion_main, Criterion};
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

#[inline(always)]
fn milstein_step(x: f64, mu: f64, sigma: f64, half_sigma_sq: f64, dt: f64, dw: f64) -> f64 {
    x + mu * x * dt + sigma * x * dw + half_sigma_sq * x * (dw * dw - dt)
}

fn run_milstein(
    dw: &[f64], x0: f64, dt: f64, n_paths: usize, n_steps: usize,
    mu: f64, sigma: f64, terminal: &mut [f64],
) {
    let half_sigma_sq = 0.5 * sigma * sigma;
    for i in 0..n_paths {
        let mut x = x0;
        let row = &dw[i * n_steps..(i + 1) * n_steps];
        for s in 0..n_steps {
            x = milstein_step(x, mu, sigma, half_sigma_sq, dt, row[s]);
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

    c.bench_function("pathwise_milstein_gbm_scalar", |b| {
        b.iter(|| {
            run_milstein(
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

    run_milstein(fx.dw(), m.x0, m.dt, m.n_paths as usize, m.n_steps as usize, mu, sigma, &mut terminal);
    let n = terminal.len() as f64;
    let mean: f64 = terminal.iter().sum::<f64>() / n;
    let var: f64 = terminal.iter().map(|v| (v - mean).powi(2)).sum::<f64>() / (n - 1.0);
    let stderr = (var / n).sqrt();

    let reps = 20usize;
    let mut ns_per_run: Vec<f64> = Vec::with_capacity(reps);
    for _ in 0..reps {
        let t0 = Instant::now();
        run_milstein(fx.dw(), m.x0, m.dt, m.n_paths as usize, m.n_steps as usize, mu, sigma, &mut terminal);
        ns_per_run.push(t0.elapsed().as_nanos() as f64);
    }
    ns_per_run.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let median_ns = ns_per_run[reps / 2];
    let path_steps = (m.n_paths * m.n_steps) as f64;
    let ns_per_path_step = median_ns / path_steps;
    let path_steps_per_s = path_steps / (median_ns * 1e-9);

    let out = results_dir();
    fs::create_dir_all(&out).expect("mkdir results");
    let path = out.join("pathwise_milstein_gbm_scalar.csv");
    let mut f = fs::File::create(&path).expect("create csv");
    writeln!(f, "impl,width,scheme,process,payoff,n_paths,n_steps,ns_per_path_step,paths_per_s,mc_mean,mc_stderr").unwrap();
    writeln!(f, "pathwise,scalar,milstein,gbm,none,{},{},{},{},{},{}", m.n_paths, m.n_steps, ns_per_path_step, path_steps_per_s, mean, stderr).unwrap();
}

criterion_group!(benches, bench);
criterion_main!(benches);
