# GCC-746: A reference used in its own initializer is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR119339](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119339)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:dcl.ref.uninitialized.reference`

## Reproducer

[`gcc-746-constexpr-reference-self-init.cpp`](gcc-746-constexpr-reference-self-init.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr int& id(int& r) { return r; }
    constexpr bool f() { int& r = id(r); (void)r; return true; }   // undefined
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  A reference member read before its mem-initializer is rejected.

Found by the D4277R1 per-UB review (2026-10-12).
