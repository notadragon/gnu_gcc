# GCC-753: delete[] through a pointer to base not rejected in constant evaluation (same-size or one-element arrays)

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.delete.dynamic.array.dynamic.type.differ`

## Reproducer

[`gcc-753-constexpr-array-delete-via-base.cpp`](gcc-753-constexpr-array-delete-via-base.cpp), at `-std=c++26 -fsyntax-only`:

    struct B { int i = 0; };
    struct D : B {};                 // same size as B
    constexpr bool f() {
      B* p = new D[2];
      delete[] p;                    // UB [expr.delete]/3
      return true;
    }
    static_assert(f());              // accepted; clang: "delete of pointer to subobject"

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Rejected only incidentally when a larger derived type makes the wrong-stride destruction leave the allocation; PR97647 covers single-object delete only.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `s "delete[] base"`, `s "array delete constant"`, `s "delete array derived"`; `q "constexpr delete[] derived base pointer array"`; `p "expr.delete"`, `p "delete of pointer to subobject"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** delete[] through a pointer to base not rejected in constant evaluation (same-size or one-element arrays)

**Description:** The program below has core-language undefined behavior ([expr.delete]; D4277R1 category `ub:expr.delete.dynamic.array.dynamic.type.differ`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Rejected only incidentally when a larger derived type makes the wrong-stride destruction leave the allocation; PR97647 covers single-object delete only.

    struct B { int i = 0; };
    struct D : B {};                 // same size as B
    constexpr bool f() {
      B* p = new D[2];
      delete[] p;                    // UB [expr.delete]/3
      return true;
    }
    static_assert(f());              // accepted; clang: "delete of pointer to subobject"

Found by the D4277R1 per-UB review (2026-10-12).
