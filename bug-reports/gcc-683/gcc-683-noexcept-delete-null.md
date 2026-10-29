# GCC-683: noexcept of a delete of a null pointer constant ignores a throwing destructor

**Status:** Open (upstream); open here
**Component:** c++
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** [PR65174](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65174)
(NEW since 2015, same defect: "noexcept() returns true when operator delete
with a NULL PTR that has a throwing destructor").  Related: PR115963 (the
same shortcut accepts a delete of a null pointer constant to an incomplete
class type, P3144R2).
**Affects:** measured 2026-10-09: stock trunk (`17.0.0 20261005`, the CE
nightly at `8802cd9fdbe`), 16.2.0, 16.1.0, 15.3.0, 14.4.0, 13.4.0 and this
branch fail the two null-constant assertions; Clang passes.

## Reproducer

[`gcc-683-noexcept-delete-null.cpp`](gcc-683-noexcept-delete-null.cpp), at
`-std=c++11 -fsyntax-only`:

    struct D { ~D () noexcept (false); };
    D *p;
    static_assert (!noexcept (delete p), "control");
    static_assert (!noexcept (delete (D *) 0), "(D *) 0");
    static_assert (!noexcept (delete static_cast<D *> (nullptr)), "nullptr");

The control passes; the other two fail.  A `constexpr D *np = nullptr;`
operand passes too: only a literal null pointer constant is affected.
`noexcept` is a static property of the operand -- the delete-expression
invokes the destructor ([except.spec]/6) -- and does not depend on the
operand's value.  The same shortcut means a private or deleted destructor
or a deleted `operator delete` is not diagnosed in `delete (P *) 0`, which
Clang rejects.

## Cause

`delete_sanity` (`decl2.cc`) returns a no-op `NOP_EXPR` for an operand that
is `integer_zerop` ("Deleting a pointer with the value zero is valid and has
no effect") before `build_delete` or `build_vec_delete` builds the
destructor call, so `check_noexcept_r` has nothing to find.

## Not fixed here

Removing the shortcut is not obviously right: PR65174's discussion questions
whether the result is wrong, and PR115963 ties the same shortcut to the
accepted delete of an incomplete type.  Kept as a defect with the watch test
`g++.dg/contracts/cpp26/open-bug-noexcept-delete-null.C`.

## Found by

The Clang fork's M17 (CLANG-665, `predicate-throw-delete.cpp`).  GCC's
runtime mirror `predicate-throw-delete.C` passes: its operand is not a null
pointer constant.
