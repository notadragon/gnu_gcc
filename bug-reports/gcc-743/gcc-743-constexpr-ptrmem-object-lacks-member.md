# GCC-743: Applying a pointer to member to an object that does not contain the member is accepted in constant evaluation when no data is read

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR121250](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121250), [PR105277](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105277)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.mptr.oper.not.contain.member`

## Reproducer

[`gcc-743-constexpr-ptrmem-object-lacks-member.cpp`](gcc-743-constexpr-ptrmem-object-lacks-member.cpp), at `-std=c++26 -fsyntax-only`:

    struct B { int dummy = 0; };
    struct D : B { int x = 0; constexpr int h() const { return 2; } };
    constexpr int f() {
      int (B::*pb)() const = static_cast<int (B::*)() const>(&D::h);  // OK
      B b;
      return (b.*pb)();          // undefined: b does not contain D::h
    }
    static_assert(f() == 2);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  An application that reads outside the object is rejected incidentally; `&(b.*pb)` for a data member is accepted too.

Found by the D4277R1 per-UB review (2026-10-12).
