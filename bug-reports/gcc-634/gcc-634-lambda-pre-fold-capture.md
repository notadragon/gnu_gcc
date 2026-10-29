# GCC-634: a fold over a captured pack in a lambda's precondition is rejected, and ICEs

**Status:** Open (upstream); fixed here
**Resolved by:** [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009)
**Component:** c++ (contracts, lambdas, variadic templates)
**Keywords (ours -- upstream sets its own):** `rejects-valid`, `ice-on-valid-code`
**Upstream Link:** None found (searched 2026-10-02: Bugzilla quicksearch for
`could not convert contract fold`, `lambda pack capture contract`).
**Affects:** measured 2026-10-02 against stock trunk (`17.0.0 20260922`),
16.2.0 and this branch.

**Bugzilla Summary:** [c++26][contracts] fold over a captured pack in a
lambda's precondition: "could not convert", then ICE

## Reproducer

[`gcc-634-lambda-pre-fold-capture.cpp`](gcc-634-lambda-pre-fold-capture.cpp),
at `-std=c++26 -fcontracts -c`:

    template<class... Ts> int p (Ts... ts)
    {
      auto l = [=] (int u) pre (((u > ts) && ...)) { return (u + ... + ts); };
      return l (5);
    }
    int x = p (1, 2);

Stock trunk and 16.2:

    error: could not convert '((const Ts ...)p::ts < ... && ...)' from
      'dependent_operator_type' ... to 'bool'
    internal compiler error: unexpected expression ... of kind unary_right_fold_expr

This branch: `internal compiler error: in expand_expr_real_1, at
expr.cc:11800`; with an explicit pack capture `[ts...]`, `internal compiler
error: in reconstruct_lambda_capture_pack, at cp/pt.cc:14573` (stock again
rejects).  The same fold in the lambda's body compiles.  Clang (with
contracts) and the EDG front end accept it.

## Found by

The EDG fork's EDG-50, cross-checked against our GCC and Clang, 2026-10-02.

## Fixed here

tsubst_lambda_expr substitutes a lambda's contracts while its operator() is being defined; tsubst_contract_specifier injected a context-less `this' there, which made the predicate look like a default member initializer where nothing is captured, and substituted into a copy of the local specializations, so the body captured the pack again.  An explicit pack capture's proxy was never bridged to the instantiation's.
