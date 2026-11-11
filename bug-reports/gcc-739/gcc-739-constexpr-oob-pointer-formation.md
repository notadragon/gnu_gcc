# GCC-739: Forming a pointer beyond one past the end of an array is accepted in constant evaluation (only subscripts and reads are rejected)

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR70151](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70151), [PR111242](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111242) (heap); PR82876 (overflow)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.add.out.of.bounds.known / .unknown`

## Reproducer

[`gcc-739-constexpr-oob-pointer-formation.cpp`](gcc-739-constexpr-oob-pointer-formation.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr int f() {
      int a[3] = {1, 2, 3};
      int* p = a + 4;          // undefined: beyond one past the end
      return *(p - 4);         // evaluated through the invalid pointer
    }
    static_assert(f() == 1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  `a[4]` and `&a[4]` are rejected; `a + 4`, `a - 1`, `p += 5`, and the heap form `&a[5]` are accepted.

Found by the D4277R1 per-UB review (2026-10-12).
