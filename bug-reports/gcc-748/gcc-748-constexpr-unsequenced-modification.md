# GCC-748: An unsequenced modification and read of a scalar is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR87275](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87275) (PR116695 duplicate)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:intro.execution.unsequenced.modification`

## Reproducer

[`gcc-748-constexpr-unsequenced-modification.cpp`](gcc-748-constexpr-unsequenced-modification.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr int f() { int i = 0; i = i++ + i; return i; }   // undefined
    static_assert(f() >= 0);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Clang accepts too (llvm#37768).

Found by the D4277R1 per-UB review (2026-10-12).
