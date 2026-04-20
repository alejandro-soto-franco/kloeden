//! Digital-delta correctness, three rows:
//! - `pathwise × digital_naive`  : naive pathwise-diff, returns 0 per path (Dirac f')
//! - `rust-hand-rolled × digital_bel`  : hand-coded constant-flow BEL weight in Rust
//! - `elworthy × digital_bel`  : elworthy-rt 0.1.3's `bel_delta_constant_flow_from_paths`
//!
//! All three read the same shared Brownian buffer. The two BEL rows integrate
//! identical paths so their mean + stderr match bit-for-bit, demonstrating that
//! the off-the-shelf `elworthy` estimator and a hand-rolled implementation
//! compute the same arithmetic on digital-delta GBM.

use criterion::{criterion_group, criterion_main, Criterion};
use elworthy_rt::from_paths::bel_delta_constant_flow_from_paths;
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

fn write_csv(
    impl_name: &str,
    payoff: &str,
    n_paths: u64,
    n_steps: u64,
    mean: f64,
    stderr: f64,
) {
    let out = results_dir();
    fs::create_dir_all(&out).expect("mkdir results");
    let path = out.join(format!("{}_{}_gbm_scalar.csv", impl_name, payoff));
    let mut f = fs::File::create(&path).expect("create csv");
    writeln!(f, "impl,width,scheme,process,payoff,n_paths,n_steps,ns_per_path_step,paths_per_s,mc_mean,mc_stderr").unwrap();
    writeln!(f, "{},scalar,euler,gbm,{},{},{},-1,-1,{},{}",
        impl_name, payoff, n_paths, n_steps, mean, stderr).unwrap();
}

fn bench(_c: &mut Criterion) {
    let fx = load(default_fixtures_dir()).expect("fixture load");
    let m = fx.meta.clone();
    let mu = 0.05_f64;
    let sigma = 0.2_f64;
    let strike = m.x0;
    let horizon = m.t;
    let bel_scale = 1.0 / (horizon * sigma * m.x0);

    // Single pass: simulate paths, collect terminals and W_T, accumulate both
    // naive (= 0 per path) and hand-rolled BEL samples.
    let mut naive_sum = 0.0_f64;
    let mut naive_sum_sq = 0.0_f64;
    let mut bel_sum = 0.0_f64;
    let mut bel_sum_sq = 0.0_f64;

    let dw = fx.dw();
    let n_paths = m.n_paths as usize;
    let n_steps = m.n_steps as usize;
    let mut terminal = Vec::with_capacity(n_paths);
    let mut w_total_vec = Vec::with_capacity(n_paths);
    for i in 0..n_paths {
        let mut x = m.x0;
        let mut w_total = 0.0_f64;
        let row = &dw[i * n_steps..(i + 1) * n_steps];
        for s in 0..n_steps {
            let dw_s = row[s];
            w_total += dw_s;
            x += mu * x * m.dt + sigma * x * dw_s;
        }
        // naive pathwise: f'(X_T) = 0 a.e.
        let naive_sample = 0.0_f64 * (x / m.x0);
        naive_sum += naive_sample;
        naive_sum_sq += naive_sample * naive_sample;

        // hand-rolled BEL constant-flow: f(X_T) * W_T / (T * sigma * S_0).
        let f = if x > strike { 1.0 } else { 0.0 };
        let bel_sample = f * w_total * bel_scale;
        bel_sum += bel_sample;
        bel_sum_sq += bel_sample * bel_sample;

        terminal.push(x);
        w_total_vec.push(w_total);
    }

    let n = n_paths as f64;
    let naive_mean = naive_sum / n;
    let naive_var = (naive_sum_sq / n) - naive_mean * naive_mean;
    let naive_stderr = (naive_var.max(0.0) / n).sqrt();
    write_csv("pathwise", "digital_naive", m.n_paths, m.n_steps, naive_mean, naive_stderr);

    let bel_mean = bel_sum / n;
    let bel_var = (bel_sum_sq / n) - bel_mean * bel_mean;
    let bel_stderr = (bel_var.max(0.0) / n).sqrt();
    write_csv("rust-hand-rolled", "digital_bel", m.n_paths, m.n_steps, bel_mean, bel_stderr);

    // elworthy-rt 0.1.3 `bel_delta_constant_flow_from_paths`: off-the-shelf
    // BEL estimator for externally-generated paths. `sigma_at_x0` for GBM
    // is sigma * S_0. Pass a hard digital closure.
    let pd = bel_delta_constant_flow_from_paths(
        &terminal,
        &w_total_vec,
        |x: f64| if x > strike { 1.0 } else { 0.0 },
        m.t,
        sigma * m.x0,
    );
    write_csv("elworthy", "digital_bel", m.n_paths, m.n_steps, pd.delta.mean, pd.delta.stderr);

    eprintln!("pathwise digital_naive:         mean={} stderr={}", naive_mean, naive_stderr);
    eprintln!("rust-hand-rolled digital_bel:   mean={} stderr={}", bel_mean, bel_stderr);
    eprintln!("elworthy digital_bel:           mean={} stderr={}", pd.delta.mean, pd.delta.stderr);
}

criterion_group!(benches, bench);
criterion_main!(benches);
