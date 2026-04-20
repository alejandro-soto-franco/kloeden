#[test]
fn smoke() {
    use elworthy_codegen::KernelCache;
    use elworthy_expr::Expr;
    use elworthy_rt::euler_scalar_jit_cached;
    let mu = Expr::c(0.05) * Expr::state(0);
    let sigma = Expr::c(0.2) * Expr::state(0);
    let payoff = Expr::state(0);
    let mut cache = KernelCache::new();
    let est = euler_scalar_jit_cached(&mut cache, &mu, &sigma, &payoff, &[], 100.0, 1.0, 256, 10_000, 20260420).expect("jit");
    eprintln!("elworthy smoke: mean={} stderr={}", est.mean, est.stderr);
}
