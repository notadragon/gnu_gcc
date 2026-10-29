# GCC-763: Binding a reference through a past-the-end pointer accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.unary.dereference (.indirection), basic.compound.invalid.pointer (.indirection)`

## Reproducer

[`gcc-763-constexpr-reference-through-invalid-pointer.cpp`](gcc-763-constexpr-reference-through-invalid-pointer.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr bool f() {
      int a[1] = {0};
      int* p = a + 1;
      int& r = *p;                   // UB [expr.unary.op]/1: p points to no object
      (void)r;
      return true;
    }
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Related PR108537, PR115592.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `p "of dereferenced one-past-the-end pointer"` (only PR107735, unrelated); `s "deallocated constant"`, `s "after delete constexpr"`, `s "use after free constant"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Binding a reference through a past-the-end pointer accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.unary.op]; D4277R1 category `ub:expr.unary.dereference`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Related PR108537, PR115592.

    constexpr bool f() {
      int a[1] = {0};
      int* p = a + 1;
      int& r = *p;                   // UB [expr.unary.op]/1: p points to no object
      (void)r;
      return true;
    }
    static_assert(f());

Found by the D4277R1 per-UB review (2026-10-12).
