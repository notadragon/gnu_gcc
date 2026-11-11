# GCC-742: `static_cast` of a pointer to member to a base class that does not contain the member is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR109243](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109243) (PR121579, PR126639 duplicates)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.static.cast.does.not.contain.original.member; conv.member.missing.member`

## Reproducer

[`gcc-742-constexpr-ptrmem-cast-unrelated-base.cpp`](gcc-742-constexpr-ptrmem-cast-unrelated-base.cpp), at `-std=c++26 -fsyntax-only`:

    struct A { int a; };
    struct B { int b; };
    struct D : A, B {};
    constexpr bool f() {
      int D::*pd = &A::a;
      int B::*pb = static_cast<int B::*>(pd);   // undefined: B does not contain A::a
      return pb != nullptr;
    }
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Member pointers are tracked as bare offsets, so neither the conversion nor (in many shapes) its application is checked; the same family covers the [conv.mem] derived-to-base direction.

Found by the D4277R1 per-UB review (2026-10-12).
