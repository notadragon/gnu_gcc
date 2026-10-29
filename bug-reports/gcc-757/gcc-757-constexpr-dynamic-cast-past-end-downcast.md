# GCC-757: dynamic_cast downcast of a past-the-end pointer accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.dynamic.cast.pointer.lifetime, expr.dynamic.cast.glvalue.lifetime`

## Reproducer

[`gcc-757-constexpr-dynamic-cast-past-end-downcast.cpp`](gcc-757-constexpr-dynamic-cast-past-end-downcast.cpp), at `-std=c++26 -fsyntax-only`:

    struct B { virtual constexpr ~B() {} };
    struct D : B {};
    constexpr int t(int x) {
      B bs[1];
      B* bp = bs + x;
      D* dp = dynamic_cast<D*>(bp);  // UB [expr.dynamic.cast]/7 for x == 1
      return dp == nullptr ? -1 : 0;
    }
    static_assert(t(1) == -1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Dangling and inactive-union operands are rejected.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `s "dynamic_cast constant"`, `s "dynamic_cast constexpr"`; `q "dynamic_cast past the end"`; `p "dynamic_cast of dereferenced one-past-the-end"`, `p "of dereferenced one-past-the-end pointer"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** dynamic_cast downcast of a past-the-end pointer accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.dynamic.cast]; D4277R1 category `ub:expr.dynamic.cast.pointer.lifetime`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Dangling and inactive-union operands are rejected.

    struct B { virtual constexpr ~B() {} };
    struct D : B {};
    constexpr int t(int x) {
      B bs[1];
      B* bp = bs + x;
      D* dp = dynamic_cast<D*>(bp);  // UB [expr.dynamic.cast]/7 for x == 1
      return dp == nullptr ? -1 : 0;
    }
    static_assert(t(1) == -1);

Found by the D4277R1 per-UB review (2026-10-12).
