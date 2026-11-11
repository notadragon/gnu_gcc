# GCC-760: Direct call to a pure virtual function with a constexpr definition from its class's constructor is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:class.abstract.pure.virtual (.direct)`

## Reproducer

[`gcc-760-constexpr-pure-virtual-direct-call.cpp`](gcc-760-constexpr-pure-virtual-direct-call.cpp), at `-std=c++26 -fsyntax-only`:

    struct A {
      constexpr virtual int f() const = 0;
      int v;
      constexpr A() : v(f()) {}      // UB [class.abstract]/6
    };
    constexpr int A::f() const { return 1; }
    struct B : A { constexpr int f() const override { return 2; } };
    constexpr int g() { B b; return b.v; }
    static_assert(g() == 1);         // passes (GCC calls A::f)

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Related PR59246 (run time: GCC calls the definition), PR124834.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `s "pure virtual constant"`, `s "pure virtual constexpr"`, `s "pure virtual constructor"`, `s "pure virtual call"`, `s "pure virtual devirtualiz"`; `p "class.abstract"`, `p "pure virtual function"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Direct call to a pure virtual function with a constexpr definition from its class's constructor is accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([class.abstract]; D4277R1 category `ub:class.abstract.pure.virtual`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Related PR59246 (run time: GCC calls the definition), PR124834.

    struct A {
      constexpr virtual int f() const = 0;
      int v;
      constexpr A() : v(f()) {}      // UB [class.abstract]/6
    };
    constexpr int A::f() const { return 1; }
    struct B : A { constexpr int f() const override { return 2; } };
    constexpr int g() { B b; return b.v; }
    static_assert(g() == 1);         // passes (GCC calls A::f)

Found by the D4277R1 per-UB review (2026-10-12).
