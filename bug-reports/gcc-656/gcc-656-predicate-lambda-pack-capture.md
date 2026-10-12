# GCC-656: a lambda in a function template's precondition cannot capture the parameter pack

**Status:** Open (upstream); fixed here
**Resolved by:** [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb)
**Component:** c++ (contracts, lambdas, variadic templates)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`contract lambda parameter pack capture`, `use of parameter outside
function body contract lambda`).
**Affects:** measured 2026-10-06: stock trunk (`17.0.0 20260922`) rejects
the template; this branch rejects the instantiation.

**Bugzilla Summary:** [c++26][contracts] lambda in a precondition cannot
capture the function's parameter pack

## Reproducer

[`gcc-656-predicate-lambda-pack-capture.cpp`](gcc-656-predicate-lambda-pack-capture.cpp),
at `-std=c++26 -fcontracts -c`:

    template <class... T>
    int g (T... xs) pre ([=] { return ((xs > 0) && ...); } ()) { return 0; }
    int a = g (1, 2);

    stock:  error: use of parameter outside function body before '>' token
    branch: error: 'xs#0' is not captured
            sorry, unimplemented: use of 'type_pack_expansion' in template

The same lambda in a `contract_assert` in the body compiles; a single
parameter captured the same way compiles.  Clang (with contracts) accepts
it.

## Notes

A pack is captured only at instantiation, once its elements are known, and
the function template's contracts are substituted by
`regenerate_decl_from_template` before the specialization's function is
started; the capture machinery does not find the specialization's
parameters as the enclosing function's there.  A lambda's own contracts in
a template had the same shape (GCC-634, fixed here), where the
substitution does run inside the function.

## Found by

Writing GCC-651's test, 2026-10-06.  Now the regression test
`g++.dg/contracts/cpp26/predicate-lambda-pack-capture.C`.

## Fixed here

The contracts are substituted with the specialization as the current function (GCC-652's fix), and `tsubst_lambda_expr` makes a lambda in a contract predicate local to that function when it would otherwise land at namespace scope, so it captures the pack's elements from it.  Regression test: `g++.dg/contracts/cpp26/predicate-lambda-pack-capture.C`.
