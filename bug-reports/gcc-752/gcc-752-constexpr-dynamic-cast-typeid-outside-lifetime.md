# GCC-752: dynamic_cast and typeid of an object outside its lifetime accepted in constant evaluation when the result is not used

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:lifetime.outside.glvalue.dynamic.cast (.typeid), lifetime.outside.pointer.dynamic.cast (.dynamic)`

## Reproducer

[`gcc-752-constexpr-dynamic-cast-typeid-outside-lifetime.cpp`](gcc-752-constexpr-dynamic-cast-typeid-outside-lifetime.cpp), at `-std=c++26 -fsyntax-only`:

    #include <memory>
    #include <typeinfo>
    struct B { constexpr virtual ~B() {} };
    struct D : B {};
    constexpr int t() {
      D d;
      B& br = d;
      d.~D();
      (void) typeid(br);              // undefined: d is outside its lifetime
      std::construct_at(&d);
      std::allocator<D> a;
      D* p = a.allocate(1);
      B* bp = std::construct_at(p);
      std::destroy_at(p);
      (void) dynamic_cast<D*>(bp);    // undefined: *p is outside its lifetime
      a.deallocate(p, 1);
      return 1;
    }
    static_assert(t() == 1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  A `dynamic_cast` of an automatic object outside its lifetime is rejected; when the result of the allocated form is used (`bool r = dynamic_cast<D*>(bp) != nullptr;`) it is not rejected either but evaluates to a null pointer (measured 2026-10-13, this branch and stock trunk); related PR107529, PR102284.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: all open c++ PRs whose summary contains constexpr, constant or consteval (404) and the open dependents of PR55004 (181), read by summary; `s "dynamic_cast constant"`, `s "dynamic_cast constexpr"`, `s "dynamic_cast lifetime"`, `s "typeid constant"`, `s "typeid constexpr"`, `s "typeid lifetime"`; `p "dynamic_cast of object outside its lifetime"`, `p "typeid applied to object"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** dynamic_cast and typeid of an object outside its lifetime accepted in constant evaluation when the result is not used

**Description:** The program below has core-language undefined behavior ([basic.life]; D4277R1 category `ub:lifetime.outside.glvalue.dynamic.cast`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  A `dynamic_cast` of an automatic object outside its lifetime is rejected; when the result of the allocated form is used (`bool r = dynamic_cast<D*>(bp) != nullptr;`) it is not rejected either but evaluates to a null pointer (measured 2026-10-13, this branch and stock trunk); related PR107529, PR102284.

    #include <memory>
    #include <typeinfo>
    struct B { constexpr virtual ~B() {} };
    struct D : B {};
    constexpr int t() {
      D d;
      B& br = d;
      d.~D();
      (void) typeid(br);              // undefined: d is outside its lifetime
      std::construct_at(&d);
      std::allocator<D> a;
      D* p = a.allocate(1);
      B* bp = std::construct_at(p);
      std::destroy_at(p);
      (void) dynamic_cast<D*>(bp);    // undefined: *p is outside its lifetime
      a.deallocate(p, 1);
      return 1;
    }
    static_assert(t() == 1);

Found by the D4277R1 per-UB review (2026-10-12).
