# GCC-758: Virtual call in a base mem-initializer not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:class.base.init.mem.fun (.polymorphic)`

## Reproducer

[`gcc-758-constexpr-virtual-call-base-mem-init.cpp`](gcc-758-constexpr-virtual-call-base-mem-init.cpp), at `-std=c++26 -fsyntax-only`:

    struct A { int a; constexpr A(int x) : a(x) {} };
    struct B : A {
      constexpr virtual int vf() const { return 1; }
      constexpr B() : A(vf()) {}     // UB [class.base.init]/16
    };
    constexpr int f() { B b; return b.a; }
    static_assert(f() == 1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Related PR126357 (non-virtual call).  `typeid(*this)` there and member calls in a constructor precondition or destructor postcondition are accepted too.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `p "class.base.init"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Virtual call in a base mem-initializer not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([class.base.init]; D4277R1 category `ub:class.base.init.mem.fun`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Related PR126357 (non-virtual call).  `typeid(*this)` there and member calls in a constructor precondition or destructor postcondition are accepted too.

    struct A { int a; constexpr A(int x) : a(x) {} };
    struct B : A {
      constexpr virtual int vf() const { return 1; }
      constexpr B() : A(vf()) {}     // UB [class.base.init]/16
    };
    constexpr int f() { B b; return b.a; }
    static_assert(f() == 1);

Found by the D4277R1 per-UB review (2026-10-12).
