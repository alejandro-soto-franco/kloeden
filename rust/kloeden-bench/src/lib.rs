//! Shared loader for the Brownian-increment fixture buffer on the Rust bench side.
//!
//! Public API:
//! - [`Meta`] mirrors the TOML sidecar.
//! - [`load`] mmaps the .bin, reads the .meta, verifies sha256, returns a borrowed slice.
//!
//! The returned slice lives for the lifetime of the returned [`Fixture`]; the mmap
//! is owned by the fixture and unmapped on drop.

use memmap2::Mmap;
use serde::Deserialize;
use sha2::{Digest, Sha256};
use std::{
    fs::File,
    io,
    path::{Path, PathBuf},
};

#[derive(Debug, Clone, Deserialize)]
pub struct Meta {
    pub seed: u64,
    pub n_paths: u64,
    pub n_steps: u64,
    pub dt: f64,
    pub t: f64,
    pub x0: f64,
    pub rng_family: String,
    pub sha256: String,
}

pub struct Fixture {
    pub meta: Meta,
    mmap: Mmap,
}

impl Fixture {
    /// Returns the raw increments as a slice of f64 of length n_paths * n_steps.
    pub fn dw(&self) -> &[f64] {
        let n = (self.meta.n_paths * self.meta.n_steps) as usize;
        let bytes: &[u8] = &self.mmap;
        assert_eq!(bytes.len(), n * 8, "bin size mismatch vs meta");
        unsafe { std::slice::from_raw_parts(bytes.as_ptr() as *const f64, n) }
    }
}

#[derive(Debug, thiserror::Error)]
pub enum LoadError {
    #[error("io error: {0}")]
    Io(#[from] io::Error),
    #[error("meta parse: {0}")]
    Toml(#[from] toml::de::Error),
    #[error("sha256 mismatch: bin={0}, meta={1}")]
    Sha256Mismatch(String, String),
    #[error("size mismatch: bin={bin_len}, expected n_paths*n_steps*8={expected}")]
    SizeMismatch { bin_len: usize, expected: usize },
}

pub fn load(dir: impl AsRef<Path>) -> Result<Fixture, LoadError> {
    let dir = dir.as_ref();
    let bin_path = dir.join("brownian_increments.bin");
    let meta_path = dir.join("brownian_increments.meta");

    let meta_text = std::fs::read_to_string(&meta_path)?;
    let meta: Meta = toml::from_str(&meta_text)?;

    let file = File::open(&bin_path)?;
    let mmap = unsafe { Mmap::map(&file)? };

    let expected = (meta.n_paths as usize) * (meta.n_steps as usize) * 8;
    if mmap.len() != expected {
        return Err(LoadError::SizeMismatch { bin_len: mmap.len(), expected });
    }

    let mut hasher = Sha256::new();
    hasher.update(&mmap[..]);
    let computed = format!("{:x}", hasher.finalize());
    if computed != meta.sha256 {
        return Err(LoadError::Sha256Mismatch(computed, meta.sha256.clone()));
    }

    Ok(Fixture { meta, mmap })
}

/// Resolve the fixtures directory relative to the workspace root.
pub fn default_fixtures_dir() -> PathBuf {
    // CARGO_MANIFEST_DIR points to rust/kloeden-bench at build time.
    // Workspace root is two levels up from that.
    let manifest = env!("CARGO_MANIFEST_DIR");
    PathBuf::from(manifest).parent().unwrap().parent().unwrap().join("fixtures")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn fixture_loads_and_matches_meta() {
        let dir = default_fixtures_dir();
        if !dir.join("brownian_increments.bin").exists() {
            eprintln!("skipping: run ./scripts/gen-fixtures.sh first");
            return;
        }
        let f = load(&dir).expect("load");
        assert_eq!(f.dw().len() as u64, f.meta.n_paths * f.meta.n_steps);
        // First increment is finite.
        assert!(f.dw()[0].is_finite());
    }
}
