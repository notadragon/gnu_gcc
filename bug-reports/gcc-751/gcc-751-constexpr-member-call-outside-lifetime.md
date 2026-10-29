# GCC-751: Member function call on an object outside its lifetime is not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:lifetime.outside.pointer.member (.call), lifetime.outside.glvalue.member`

## Reproducer

[`gcc-751-constexpr-member-call-outside-lifetime.cpp`](gcc-751-constexpr-member-call-outside-lifetime.cpp), at `-std=c++26 -fsyntax-only`:

    #include <memory>
    struct S { int i = 0; constexpr int g() const { return 7; } constexpr ~S() {} };
    constexpr int f() {
      S s;
      s.~S();
      int r = s.g();       // UB [basic.life]/7: member call outside lifetime
      std::construct_at(&s);
      return r;
    }
    static_assert(f() == 7);   // accepted; clang: "member call on object outside its lifetime"

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Data-member reads and writes are rejected; member function calls are not.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: all open c++ PRs whose summary contains constexpr, constant or consteval (404) and the open dependents of PR55004 (181), read by summary; `s "member function lifetime"`, `s "member call lifetime"`, `s "explicit destructor constant"`, `s "explicit destructor constexpr"`; `p "member call on object outside its lifetime"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Member function call on an object outside its lifetime is not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([basic.life]; D4277R1 category `ub:lifetime.outside.pointer.member`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Data-member reads and writes are rejected; member function calls are not.

    #include <memory>
    struct S { int i = 0; constexpr int g() const { return 7; } constexpr ~S() {} };
    constexpr int f() {
      S s;
      s.~S();
      int r = s.g();       // UB [basic.life]/7: member call outside lifetime
      std::construct_at(&s);
      return r;
    }
    static_assert(f() == 7);   // accepted; clang: "member call on object outside its lifetime"

Found by the D4277R1 per-UB review (2026-10-12).
