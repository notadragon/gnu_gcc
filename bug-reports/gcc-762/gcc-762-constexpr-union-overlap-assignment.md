# GCC-762: Overlapping built-in assignment between union members not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.assign.overlap`

## Reproducer

[`gcc-762-constexpr-union-overlap-assignment.cpp`](gcc-762-constexpr-union-overlap-assignment.cpp), at `-std=c++26 -fsyntax-only`:

    union U { int i; short s; };
    constexpr int f() {
      U u;
      u.s = 1;
      u.i = u.s;                     // UB [expr.assign]/8: inexact overlap
      return u.i;
    }
    static_assert(f() == 1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Clang accepts too; related PR117679 (class-type union copy).

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `s "overlap constant"`, `s "overlapping union"`, `s "union assignment constexpr"`, `p "expr.assign"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Overlapping built-in assignment between union members not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.assign]; D4277R1 category `ub:expr.assign.overlap`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Clang accepts too; related PR117679 (class-type union copy).

    union U { int i; short s; };
    constexpr int f() {
      U u;
      u.s = 1;
      u.i = u.s;                     // UB [expr.assign]/8: inexact overlap
      return u.i;
    }
    static_assert(f() == 1);

Found by the D4277R1 per-UB review (2026-10-12).
