# GCC-759: Conversion of this to a virtual base whose deriving base has not started construction accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:class.cdtor.convert.pointer, class.cdtor.typeid, class.cdtor.virtual.not.x, class.cdtor.form.pointer, class.cdtor.after.dtor (.address)`

## Reproducer

[`gcc-759-constexpr-cdtor-convert-typeid-virtual.cpp`](gcc-759-constexpr-cdtor-convert-typeid-virtual.cpp), at `-std=c++26 -fsyntax-only`:

    struct A { int tag = 1; };
    struct CBase : virtual A {};
    struct DBase : virtual A { A* p; constexpr DBase(A* a) : p(a) {} };
    struct E : CBase, DBase {
      constexpr E() : DBase(this) {}   // UB [class.cdtor]/3: DBase not yet constructed
    };
    constexpr int t() { E e; return e.p == static_cast<A*>(&e); }
    static_assert(t());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Sibling of PR93096 (dynamic_cast) and PR126358 (form.pointer with a write).  `typeid` and a virtual call through the not-yet-constructed path are accepted; GCC and Clang disagree on their results.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `p "class.cdtor"`; `s "virtual base constant"`, `s "virtual base constexpr"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Conversion of this to a virtual base whose deriving base has not started construction accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([class.cdtor]; D4277R1 category `ub:class.cdtor.convert.pointer`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Sibling of PR93096 (dynamic_cast) and PR126358 (form.pointer with a write).  `typeid` and a virtual call through the not-yet-constructed path are accepted; GCC and Clang disagree on their results.

    struct A { int tag = 1; };
    struct CBase : virtual A {};
    struct DBase : virtual A { A* p; constexpr DBase(A* a) : p(a) {} };
    struct E : CBase, DBase {
      constexpr E() : DBase(this) {}   // UB [class.cdtor]/3: DBase not yet constructed
    };
    constexpr int t() { E e; return e.p == static_cast<A*>(&e); }
    static_assert(t());

Found by the D4277R1 per-UB review (2026-10-12).
