//! Writes fixtures/brownian_increments.bin (raw f64 LE, row-major n_paths * n_steps)
//! + fixtures/brownian_increments.meta (TOML sidecar with seed, dims, sha256).
//!
//! Increments are dW_n = sqrt(dt) * Z_n, Z_n ~ N(0, 1).
//! RNG: rand_chacha::ChaCha12Rng, seeded deterministically from --seed.

use rand::SeedableRng;
use rand_chacha::ChaCha12Rng;
use rand_distr::{Distribution, Normal};
use serde::Serialize;
use sha2::{Digest, Sha256};
use std::{
    env, fs,
    io::{self, Write},
    path::{Path, PathBuf},
    process::ExitCode,
};

#[derive(Serialize)]
struct Meta {
    seed: u64,
    n_paths: u64,
    n_steps: u64,
    dt: f64,
    t: f64,
    x0: f64,
    rng_family: String,
    sha256: String,
}

struct Args {
    seed: u64,
    n_paths: u64,
    n_steps: u64,
    t: f64,
    x0: f64,
    out_dir: PathBuf,
}

fn parse_args() -> Result<Args, String> {
    let mut seed: u64 = 20260420;
    let mut n_paths: u64 = 10_000;
    let mut n_steps: u64 = 256;
    let mut t: f64 = 1.0;
    let mut x0: f64 = 100.0;
    let mut out_dir: PathBuf = PathBuf::from("fixtures");

    let mut it = env::args().skip(1);
    while let Some(flag) = it.next() {
        let val = it.next().ok_or_else(|| format!("{flag} requires a value"))?;
        match flag.as_str() {
            "--seed" => seed = val.parse().map_err(|e| format!("--seed: {e}"))?,
            "--n-paths" => n_paths = val.parse().map_err(|e| format!("--n-paths: {e}"))?,
            "--n-steps" => n_steps = val.parse().map_err(|e| format!("--n-steps: {e}"))?,
            "--t" => t = val.parse().map_err(|e| format!("--t: {e}"))?,
            "--x0" => x0 = val.parse().map_err(|e| format!("--x0: {e}"))?,
            "--out-dir" => out_dir = PathBuf::from(val),
            other => return Err(format!("unknown flag {other}")),
        }
    }

    Ok(Args { seed, n_paths, n_steps, t, x0, out_dir })
}

fn write_buffer(args: &Args) -> io::Result<Meta> {
    fs::create_dir_all(&args.out_dir)?;
    let bin_path = args.out_dir.join("brownian_increments.bin");
    let meta_path = args.out_dir.join("brownian_increments.meta");

    let dt = args.t / args.n_steps as f64;
    let sqrt_dt = dt.sqrt();
    let normal = Normal::new(0.0, 1.0).unwrap();
    let mut rng = ChaCha12Rng::seed_from_u64(args.seed);

    let total = (args.n_paths as usize) * (args.n_steps as usize);
    let mut buf = Vec::with_capacity(total * 8);
    let mut hasher = Sha256::new();

    for _ in 0..total {
        let z: f64 = normal.sample(&mut rng);
        let dw = sqrt_dt * z;
        let bytes = dw.to_le_bytes();
        buf.extend_from_slice(&bytes);
        hasher.update(&bytes);
    }

    fs::write(&bin_path, &buf)?;
    let sha256 = format!("{:x}", hasher.finalize());

    let meta = Meta {
        seed: args.seed,
        n_paths: args.n_paths,
        n_steps: args.n_steps,
        dt,
        t: args.t,
        x0: args.x0,
        rng_family: "chacha12-rand-0.9".to_string(),
        sha256,
    };
    let meta_text = toml::to_string_pretty(&meta).unwrap();
    fs::write(&meta_path, meta_text)?;

    eprintln!("wrote {} ({} bytes)", bin_path.display(), buf.len());
    eprintln!("wrote {}", meta_path.display());
    Ok(meta)
}

fn main() -> ExitCode {
    let args = match parse_args() {
        Ok(a) => a,
        Err(e) => {
            let _ = writeln!(io::stderr(), "kloeden-buffer-gen: {e}");
            return ExitCode::from(2);
        }
    };

    match write_buffer(&args) {
        Ok(_) => ExitCode::SUCCESS,
        Err(e) => {
            let _ = writeln!(io::stderr(), "kloeden-buffer-gen: {e}");
            ExitCode::FAILURE
        }
    }
}

// Silence dead-code warning for unused helper.
#[allow(dead_code)]
fn _path_exists(p: &Path) -> bool { p.exists() }
