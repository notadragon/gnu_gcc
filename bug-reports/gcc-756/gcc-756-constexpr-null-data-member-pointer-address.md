# GCC-756: &(s.*pm) with a null pointer to data member accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.mptr.oper.member.func.null (.data)`

## Reproducer

[`gcc-756-constexpr-null-data-member-pointer-address.cpp`](gcc-756-constexpr-null-data-member-pointer-address.cpp), at `-std=c++26 -fsyntax-only`:

    struct S { int m = 1; };
    constexpr bool f() {
      int S::*pm = nullptr;
      S s;
      int* p = &(s.*pm);             // UB [expr.mptr.oper]/6
      return p != nullptr;
    }
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Related PR126360 (discarded `s.*pm;`).

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `s "null member pointer"`, `s "null pointer to member"`, `s "null pointer-to-member"`; `q "constexpr null data member pointer dereference"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** &(s.*pm) with a null pointer to data member accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.mptr.oper]; D4277R1 category `ub:expr.mptr.oper.member.func.null`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Related PR126360 (discarded `s.*pm;`).

    struct S { int m = 1; };
    constexpr bool f() {
      int S::*pm = nullptr;
      S s;
      int* p = &(s.*pm);             // UB [expr.mptr.oper]/6
      return p != nullptr;
    }
    static_assert(f());

Found by the D4277R1 per-UB review (2026-10-12).
