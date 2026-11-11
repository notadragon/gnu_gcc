# GCC-785: `&*(a + N)`, an indirection through a past-the-end pointer, accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-13) -- filing owed; related PR108537
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`), 16.2.0, 15.3.0, 14.4.0; this branch's install of 2026-10-13
**D4277R1 category:** `ub:expr.unary.dereference (.invalid.known)`

## Reproducer

[`gcc-785-constexpr-address-past-end-indirection.cpp`](gcc-785-constexpr-address-past-end-indirection.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr bool f() { int a[1] = {0}; int* p = &*(a + 1); return p != nullptr; }   // UB [expr.unary.op]/1
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression
is wrongly treated as a constant expression).  The unary `*` yields an
lvalue denoting the object the operand points to ([expr.unary.op]/1), and
`a + 1` points to none.  Reading through it is rejected.  Binding a
reference to it is GCC-763.  Clang accepts it too (llvm_llvm-project
CLANG-712, llvm#65119, closed not_planned there: a maintainer considers
`&a[N]` well-defined).

## Upstream search (2026-10-13)

Bugzilla REST, component c++, control validated (the summary phrase
"Calling member function before base subobject" returned PR126357).
Queries: `s "one-past-the-end constexpr"`, `s "past-the-end constant"`,
`s "past the end dereference"` (only PR67376, fixed, unrelated).
PR108537 (a discarded `b[100]` past the end accepted) is related.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Indirection through a past-the-end pointer accepted in constant evaluation when only its address is taken
**Description:** The program below has core-language undefined behavior ([expr.unary.op]; D4277R1 category `ub:expr.unary.dereference`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Related PR108537.

    constexpr bool f() { int a[1] = {0}; int* p = &*(a + 1); return p != nullptr; }
    static_assert(f());

Found by the CLANG-712 mirror (M52-GCC, 2026-10-13).
