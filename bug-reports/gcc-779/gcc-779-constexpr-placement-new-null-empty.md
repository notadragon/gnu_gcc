# GCC-779: A placement new of an empty class at a null pointer is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-13) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`), 16.2.0, 16.1.0, 15.3.0; this branch's install of 2026-10-13 (14.4.0 rejects the program, as constexpr placement new is C++26, P2747R2, implemented in 15)
**D4277R1 category:** `ub:expr.new.non.allocating.null`

## Reproducer

[`gcc-779-constexpr-placement-new-null-empty.cpp`](gcc-779-constexpr-placement-new-null-empty.cpp), at `-std=c++26 -fsyntax-only`:

    #include <new>
    struct E {};
    constexpr bool f() { E* p = nullptr; new (p) E; return true; }   // UB [expr.new]
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is
wrongly treated as a constant expression).  The non-allocating placement
`operator new` returns its argument, a null pointer, and a new-expression
whose non-allocating allocation function returns null has undefined
behavior ([expr.new]).  The default-initialization of an empty class stores
nothing, so the evaluator never touches `*p`: `new (p) E ()`
(value-initialization) is rejected ("modification of `*0` from outside
current evaluation"), and so is `new (p) int` on 16.1 and later ("destroying
`*0`"; 15.3 accepts that one too).

On this branch, configuring `ub:expr.new.non.allocating.null` (GCC-728's
check) rejects the program as undefined; unconfigured, the check is not
built and the stock behaviour shows.  Clang rejects it either way
(llvm_llvm-project `clang/test/Contracts/p3100-placement-null-constexpr.cpp`,
which is the mirror).

## Upstream search (2026-10-13)

Bugzilla REST, product gcc, control validated (the summary phrase "Calling
member function before base subobject" returned PR126357).  Summary queries
`placement new null`, `placement new nullptr`, `placement new constexpr`,
`placement new empty`, and quicksearch `placement new constexpr null`,
`constexpr placement new empty class`, `placement new nullptr`: only
PR119304 (an invalid placement new over a `const` object accepted, open),
PR115744 (P2747R2, fixed), PR61105 (a diagnostic), unrelated.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Placement new of an empty class at a null pointer accepted in constant evaluation
**Description:** The program below has core-language undefined behavior ([expr.new]: a non-allocating allocation function returns null; D4277R1 category `ub:expr.new.non.allocating.null`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted since constexpr placement new (P2747R2) landed in GCC 15.  With `new (p) E ()` it is rejected.

    #include <new>
    struct E {};
    constexpr bool f() { E* p = nullptr; new (p) E; return true; }
    static_assert(f());

Found by the M50-Clang mirror of GCC-728 (2026-10-13) and measured by M54-GCC.
