# GCC-741: `static_cast` of an out-of-range integral value to an unfixed enumeration is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR95701](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95701) (DR1766; PR117963, PR106294 duplicates; PR126353 the same, not yet marked)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.static.cast.enum.outside.range (.integral)`

## Reproducer

[`gcc-741-constexpr-enum-out-of-range.cpp`](gcc-741-constexpr-enum-out-of-range.cpp), at `-std=c++26 -fsyntax-only`:

    enum A { a0, a1, a2, a3 };                 // values [0, 3]
    constexpr A x = static_cast<A>(4);         // undefined

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  The floating form is rejected with a poor message (GCC-716).

Found by the D4277R1 per-UB review (2026-10-12).
