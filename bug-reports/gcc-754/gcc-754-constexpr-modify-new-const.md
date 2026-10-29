# GCC-754: Modification of an object created by new const T is not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:dcl.type.cv.modify.const.obj (.dynamic)`

## Reproducer

[`gcc-754-constexpr-modify-new-const.cpp`](gcc-754-constexpr-modify-new-const.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr int f() {
      const int* p = new const int(3);
      *const_cast<int*>(p) = 4;      // UB [dcl.type.cv]/4
      int r = *p;
      delete p;
      return r;
    }
    static_assert(f() == 4);         // passes

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Automatic and static const objects are rejected (PR91264, PR85125 fixed).

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `p "modification of object of const-qualified type"`, `s "modifying const"`, `s "heap const"`, `s "allocated const object"`, `s "new const"`, `q "constexpr new const modify"`, `p "new const int"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Modification of an object created by new const T is not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([dcl.type.cv]; D4277R1 category `ub:dcl.type.cv.modify.const.obj`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Automatic and static const objects are rejected (PR91264, PR85125 fixed).

    constexpr int f() {
      const int* p = new const int(3);
      *const_cast<int*>(p) = 4;      // UB [dcl.type.cv]/4
      int r = *p;
      delete p;
      return r;
    }
    static_assert(f() == 4);         // passes

Found by the D4277R1 per-UB review (2026-10-12).
