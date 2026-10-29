# GCC-744: Reading a volatile object through a non-volatile glvalue is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR126359](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126359)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:dcl.type.cv.access.volatile`

## Reproducer

[`gcc-744-constexpr-volatile-through-nonvolatile.cpp`](gcc-744-constexpr-volatile-through-nonvolatile.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr int f() {
      volatile int v = 1;
      return const_cast<int&>(v);   // undefined: volatile object accessed through non-volatile glvalue
    }
    static_assert(f() == 1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Clang rejects ("read of volatile object ... not allowed").

Found by the D4277R1 per-UB review (2026-10-12).
