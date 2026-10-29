# GCC-749: Indirection through a null pointer that is not followed by a read is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR115592](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115592) (CWG2823), [PR126360](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126360), [PR113047](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113047)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12; the null-pointer-to-function forms also 16.2.0, 15.3.0, 14.4.0, 13.4.0 (`-std=c++2b`), measured 2026-10-14
**D4277R1 category:** `ub:expr.unary.dereference.nullptr (.indirection)`

## Reproducer

[`gcc-749-constexpr-null-indirection-no-read.cpp`](gcc-749-constexpr-null-indirection-no-read.cpp), at `-std=c++26 -fsyntax-only`:

    struct S { static constexpr int sf() { return 1; } static constexpr int k = 1; };
    constexpr bool f() {
      int* p = nullptr;
      *p;                       // undefined: indirection through a null pointer
      S* sp = nullptr;
      sp->sf();                 // undefined: *sp is evaluated
      (void) sp->k;             // undefined: *sp is evaluated
      return true;
    }
    static_assert(f());
    using F = int();
    constexpr bool faddr(F* fp) { return &*fp == nullptr; }
    static_assert(faddr(nullptr));      // undefined: *fp with fp null
    constexpr int fdisc(F* fp) { *fp; return 1; }
    static_assert(fdisc(nullptr) == 1); // undefined: *fp with fp null

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Binding `int& r = *p` is rejected; `*p;`, `&*p`, a static member function call and a static data member named through `->` (`sp->k`) are accepted.

The same holds for a null pointer to function: `&*fp` and a discarded
`*fp;` are accepted (stock GCC 13.4..trunk), while a reference bound to
`*fp` (`F &r = *fp;`, "dereferencing a null pointer") and a call through
the null pointer (`(*fp) ()`, "does not designate a constexpr function")
are rejected.  The cause is the same: `&*fp` folds to `fp` in
`cp_build_addr_expr_1` and a discarded `*fp` is dropped by
`convert_to_void`, both at parse time, so the evaluator never sees the
indirection.  No upstream PR names the function-pointer forms (searched
2026-10-14: quicksearch `function pointer constexpr null`, `null function
pointer constant`, `constexpr dereference null function` found PR102420,
PR126360, PR113047, PR97230, none about a pointer to function); a report
for them should mention PR126360.

On this branch, configuring `ub:expr.unary.dereference.nullptr.indirection`
builds GCC-799's run-time check of `&*p`, `*p;`, `&*fp` and `*fp;`, but
manifestly constant evaluation ignores it, so the program is accepted
configured too.

The member forms are also CLANG-737 in the llvm_llvm-project fork: Clang accepts `sp->sf()` and `sp->k`, and rejects `*p;` and `&*p` (since Clang 22).  The function forms are CLANG-756 there: Clang accepts `&*fp`, `*fp;` and the reference binding.

Watch test: `gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-null-indirection-no-read.C`.

Found by the D4277R1 per-UB review (2026-10-12); the function forms by the
M63-Clang measurement of CLANG-756 (2026-10-14), measured on GCC by M64-GCC.
