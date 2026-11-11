# GCC-747: `dynamic_cast` during construction through a not-yet-constructed path is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR93096](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93096) (the standard's own example)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:class.cdtor.dynamic.cast`

## Reproducer

[`gcc-747-constexpr-cdtor-dynamic-cast.cpp`](gcc-747-constexpr-cdtor-dynamic-cast.cpp), at `-std=c++26 -fsyntax-only`:

    struct V { virtual void f() {} };
    struct A : virtual V {};
    struct Bx : virtual V {
      bool ok;
      constexpr Bx(A* a) : ok(dynamic_cast<Bx*>(a) != nullptr) {} // undefined [class.cdtor]
    };
    struct D : A, Bx { constexpr D() : Bx(this) {} };
    static_assert((D{}, true));

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  

Found by the D4277R1 per-UB review (2026-10-12).
