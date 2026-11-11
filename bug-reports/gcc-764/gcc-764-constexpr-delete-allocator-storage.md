# GCC-764: `delete` of storage obtained from `std::allocator::allocate` is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.delete.mismatch`

## Reproducer

[`gcc-764-constexpr-delete-allocator-storage.cpp`](gcc-764-constexpr-delete-allocator-storage.cpp), at `-std=c++26 -fsyntax-only`:

    #include <memory>
    constexpr bool f() {
      std::allocator<int> a;
      int* p = a.allocate(1);
      delete p;                 // undefined: not from a non-array new-expression
      return true;
    }
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Clang rejects ("'delete' used to delete pointer to object allocated with 'std::allocator<...>::allocate'").  PR95797 is the converse (new, then allocator deallocate).

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `s "allocator delete constexpr"`, `s "allocate delete"`, `p "used to delete pointer to object allocated with"` (finds only PR95797/PR98052, the converse).

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** `delete` of storage obtained from `std::allocator::allocate` is accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.delete]; D4277R1 category `ub:expr.delete.mismatch`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Clang rejects ("'delete' used to delete pointer to object allocated with 'std::allocator<...>::allocate'").  PR95797 is the converse (new, then allocator deallocate).

    #include <memory>
    constexpr bool f() {
      std::allocator<int> a;
      int* p = a.allocate(1);
      delete p;                 // undefined: not from a non-array new-expression
      return true;
    }
    static_assert(f());

Found by the D4277R1 per-UB review (2026-10-12).
