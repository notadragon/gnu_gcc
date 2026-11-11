# GCC-750: Conversion to a virtual base of a named object after its destructor is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:lifetime.outside.glvalue.virtual / .pointer.virtual (.named)`

## Reproducer

[`gcc-750-constexpr-vbase-conversion-destroyed-named.cpp`](gcc-750-constexpr-vbase-conversion-destroyed-named.cpp), at `-std=c++26 -fsyntax-only`:

    #include <memory>
    struct V { int v = 1; };
    struct D : virtual V { constexpr ~D() {} };
    constexpr bool f() {
      D d;
      d.~D();
      V& r = d;            // UB [basic.life]/8: d is outside its lifetime
      (void)r;
      std::construct_at(&d);
      return true;
    }
    static_assert(f());    // accepted; should be rejected

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Converting through an unnamed glvalue (unknown most-derived type) or heap storage is rejected; for a named complete object GCC folds the virtual-base offset and never checks lifetime (the mechanism of GCC-2).

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: all open c++ PRs whose summary contains constexpr, constant or consteval (404) and the open dependents of PR55004 (181), read by summary; `s "virtual base constant"`, `s "virtual base constexpr"`; `p "conversion to a virtual base of object outside its lifetime"`; C1/C2.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Conversion to a virtual base of a named object after its destructor is accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([basic.life]; D4277R1 category `ub:lifetime.outside.glvalue.virtual`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Converting through an unnamed glvalue (unknown most-derived type) or heap storage is rejected; for a named complete object GCC folds the virtual-base offset and never checks lifetime (the mechanism of GCC-2).

    #include <memory>
    struct V { int v = 1; };
    struct D : virtual V { constexpr ~D() {} };
    constexpr bool f() {
      D d;
      d.~D();
      V& r = d;            // UB [basic.life]/8: d is outside its lifetime
      (void)r;
      std::construct_at(&d);
      return true;
    }
    static_assert(f());    // accepted; should be rejected

Found by the D4277R1 per-UB review (2026-10-12).
