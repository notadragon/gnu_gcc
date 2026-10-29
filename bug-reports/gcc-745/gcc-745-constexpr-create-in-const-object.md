# GCC-745: Creating an object in the storage of a const complete object is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR119304](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119304)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:creating.within.const.complete.obj`

## Reproducer

[`gcc-745-constexpr-create-in-const-object.cpp`](gcc-745-constexpr-create-in-const-object.cpp), at `-std=c++26 -fsyntax-only`:

    #include <memory>
    struct B { int x; };
    constexpr int f() {
      const B b{1};
      std::construct_at(const_cast<B*>(&b), 2);  // undefined: new object in a const object's storage
      return b.x;
    }
    static_assert(f() == 2);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Clang rejects ("modification of object of const-qualified type").

Found by the D4277R1 per-UB review (2026-10-12).
