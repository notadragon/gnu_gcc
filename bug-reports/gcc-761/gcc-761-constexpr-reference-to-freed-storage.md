# GCC-761: Using a reference whose referent's storage has been deallocated is not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.type.reference.lifetime`

## Reproducer

[`gcc-761-constexpr-reference-to-freed-storage.cpp`](gcc-761-constexpr-reference-to-freed-storage.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr bool f() {
      int* ip = new int(1);
      int& i = *ip;
      delete ip;
      int& j = i;                    // UB [basic.compound]/6, [expr.type]
      (void)j;
      return true;
    }
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Clang accepts too.  PR96630, PR70331 cover reads.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: all open c++ PRs whose summary contains constexpr, constant or consteval (404) and the open dependents of PR55004 (181), read by summary; C1; `p "use of reference outside its lifetime"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Using a reference whose referent's storage has been deallocated is not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.type]; D4277R1 category `ub:expr.type.reference.lifetime`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Clang accepts too.  PR96630, PR70331 cover reads.

    constexpr bool f() {
      int* ip = new int(1);
      int& i = *ip;
      delete ip;
      int& j = i;                    // UB [basic.compound]/6, [expr.type]
      (void)j;
      return true;
    }
    static_assert(f());

Found by the D4277R1 per-UB review (2026-10-12).
