# GCC-740: Subtracting pointers to different members (or rows) of one object is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR126060](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126060) (rows; the members form is not shown there)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.add.sub.diff.pointers`

## Reproducer

[`gcc-740-constexpr-sub-same-object.cpp`](gcc-740-constexpr-sub-same-object.cpp), at `-std=c++26 -fsyntax-only`:

    struct M { int a, b; };
    constexpr long f() { M m{}; return &m.b - &m.a; }   // undefined: not elements of one array
    static_assert(f() == 1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Pointers into different arrays are rejected.

Found by the D4277R1 per-UB review (2026-10-12).
