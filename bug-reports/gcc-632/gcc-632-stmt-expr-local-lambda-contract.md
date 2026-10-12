# GCC-632: a local declared in a statement expression in a lambda's contract predicate ICEs

**Status:** Open (upstream); fixed here
**Resolved by:** [e9e73c8f9403](https://github.com/notadragon/gnu_gcc/commit/e9e73c8f940326dcbaf782305ca41519788df99e)
**Component:** c++ (contracts, lambdas, GNU statement expressions)
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** None found (searched 2026-10-01: Bugzilla quicksearch for
`gimplify_var_or_parm_decl contract`, `statement expression contract
lambda` and `contracts statement-expression`).  Related, not the same:
PR126038 (a lambda's precondition naming one of its captures, the same
assertion; fixed on this branch by `999937c53c7`, and this case still ICEs
there) and PR123413 (a statement expression in a template's
`decltype(auto)` initializer).
**Affects:** measured 2026-10-01 against stock trunk (`17.0.0 20260922`),
16.2.0, 16.1.0 and this branch: identical on all four (`gimplify.cc:3298`
on trunk, `:3426` on 16.x).  15.3.0 has no contracts.

**Bugzilla Summary:** [c++26][contracts] ICE in gimplify_var_or_parm_decl
for a variable declared in a statement expression in a lambda's pre or post

## Reproducer

[`gcc-632-stmt-expr-local-lambda-contract.cpp`](gcc-632-stmt-expr-local-lambda-contract.cpp),
at `-std=c++26 -fcontracts -c`:

    void f ()
    {
      auto l = [] (const int x) pre (({ int y = x; y > 0; })) {};
      l (1);
    }

    internal compiler error: in gimplify_var_or_parm_decl, at gimplify.cc:3298

The same for `post`, for a declaration that does not use the parameter
(`({ int y = 1; y > 0; })`), and for a lambda with a deduced return type
naming its result (`post (r: ({ int z = r; z > 0; }))`).  A statement
expression without a declaration compiles (the reproducer's control), and
so does one declaring a local in a `contract_assert` in the lambda's body.
A statement expression is allowed in a lambda's contract predicate because
the lambda is in a function; a function at namespace scope or a member of a
local class cannot have one ("statement-expressions are not allowed outside
functions").  `-fsyntax-only` does not reach the ICE.

Clang (with contracts) compiles the precondition case; it has its own crash
for the deduced-return-type postcondition case (CLANG-626 in the Clang
fork).  The EDG front end rejects a statement expression in a lambda's
predicate today (its EDG-4).

## Notes

The assertion catches a `VAR_DECL` the gimplifier has not seen declared in
any `BIND_EXPR`.  The predicate is emitted as a copy (`copy_contracts_list`,
a contracts.cc helper over `copy_tree_body_r`) whose `copy_decl` hook keeps
every declaration as it is, and `remap_decls` builds the copied
`BIND_EXPR`'s variable list only from the declarations it newly made, so
the statement expression's variable drops out of it.  (Upstream has the
same helper.)

## Found by

Cross-checking the EDG fork's work on EDG-4 (statement expressions in a
lambda's contract predicate) against our GCC and Clang, 2026-10-01.

## Fixed here

copy_contracts_list copied each emitted predicate with a copy_decl hook that kept every declaration, and remap_decls rebuilds a BIND_EXPR's variable list from the declarations it newly made, so the statement expression's variable dropped out of its own BIND_EXPR.  A variable the predicate declares is now copied.  With a deduced return type the predicate is parsed as if in a template with no template parameters; start_decl then lost the variable to push_template_decl, which is no longer called for a local there.
