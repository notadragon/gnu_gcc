# GCC-714: Double destruction of an automatic object is accepted in constant evaluation when the destructor does not touch the object

**Status:** Open here; open upstream
**Component:** c++
**Upstream Link:** [PR102284](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102284) (comment 2 is this case)
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build), this branch's release install of 2026-10-12
**Bugzilla Summary:** (existing PR102284)

## Reproducer

[`gcc-714-constexpr-double-destruction-automatic.cpp`](gcc-714-constexpr-double-destruction-automatic.cpp), at `-std=c++26 -fsyntax-only`:

    struct X { constexpr ~X() {} };
    constexpr bool f() {
      X x;
      x.~X();
      return true;        // x is destroyed again at scope exit: undefined
    }
    static_assert(f());

Accepted by stock trunk back to 13.4.0 and by this branch; Clang rejects
("destroying object 'x' whose lifetime has already ended").  GCC rejects only
when the second destructor writes the object or the class is polymorphic (the
vptr store).  The GCC-69/70 fix covers heap objects only.  This is the
`ub:class.dtor.no.longer.exists` description example.

Found by the D4277R1 per-UB review (2026-10-12).
