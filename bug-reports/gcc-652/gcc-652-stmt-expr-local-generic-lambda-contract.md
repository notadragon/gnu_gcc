# GCC-652: a statement expression's variable in a generic lambda's contract is rejected at instantiation

**Status:** Open (upstream); fixed here
**Resolved by:** [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb)
**Component:** c++ (contracts, generic lambdas, GNU statement expressions)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`generic lambda contract statement expression`, `use of local variable
with automatic storage from containing function contract`).
**Affects:** measured 2026-10-06 against stock trunk (`17.0.0 20260922`)
and this branch: identical.

**Bugzilla Summary:** [c++26][contracts] statement expression local in a
generic lambda's precondition: "use of local variable with automatic
storage from containing function"

## Reproducer

[`gcc-652-stmt-expr-local-generic-lambda-contract.cpp`](gcc-652-stmt-expr-local-generic-lambda-contract.cpp),
at `-std=c++26 -fcontracts -c`:

    int k ()
    {
      auto m = [] (auto x) pre (({ int y = x; y > 0; })) { return x; };
      return m (1);
    }

    error: use of parameter from containing function
    error: use of local variable with automatic storage from containing function

The same for a postcondition naming the result.  A non-generic lambda
compiles on this branch (GCC-632, fixed here), as does one in a function
template.  Clang (with contracts) accepts it.

## Notes

The specialization's contracts are substituted by
`regenerate_decl_from_template`, before `instantiate_body` starts the
specialization's function, so the statement expression's variable is
created and its uses checked outside that function.  The generic lambda's
captures in a predicate had the same ordering problem (GCC-633, fixed here
by naming the closure member instead of the proxy); a local declared in the
predicate needs the function to exist.

## Found by

Testing the shapes of GCC-632's fix, 2026-10-06.  Now the regression test
`g++.dg/contracts/cpp26/stmt-expr-local-generic-lambda.C`.

## Fixed here

`tsubst_contract_specifier` substitutes a specialization's contracts with the specialization as the current function (and its injected `this` given that context, so it is not taken for a default member initializer's), as the predicate was parsed; the variable then belongs to the operator(), however early the substitution runs.  A postcondition of a generic lambda with a deduced return type still ICEs: GCC-660, in `open-issues/`.  Regression test: `g++.dg/contracts/cpp26/stmt-expr-local-generic-lambda.C`.
