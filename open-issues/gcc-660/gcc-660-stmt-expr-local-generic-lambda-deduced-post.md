# GCC-660: a statement expression's variable in a deduced generic lambda's postcondition ICEs

**Status:** Open, this branch (stock GCC rejects every such variable in a
generic lambda's contract: GCC-652, fixed here except for this shape).
**Kind:** defect.
**Affects:** measured 2026-10-06 on the M5-GCC working tree.
**Watch test:** `g++.dg/contracts/cpp26/open-bug-stmt-expr-local-generic-lambda-deduced-post.C`.

## Reproducer

    int k (int v)
    {
      auto m = [] (auto x) post (r: __extension__ ({ int z = r; z > 0; }))
      { return x; };
      return m (v);
    }

    $ g++ -std=c++26 -fcontracts -c t.cpp
    internal compiler error: in gimplify_var_or_parm_decl, at gimplify.cc:3298

With `-> int` it compiles and checks (`stmt-expr-local-generic-lambda.C`),
as does a non-generic lambda with a deduced return type
(`stmt-expr-local-lambda-deduced-post.C`).

## Why (as far as measured)

The emitted check is `if (z <= 0)` with no declaration of `z` left
(`-fdump-tree-original`): the statement expression has lost its
statements.  While the return type is undeduced the specialization's
predicate is substituted as if in a template (`tsubst_contract`), and
`rebuild_postconditions` substitutes it again once the body's return
statement deduces the type; somewhere between the two the statement
expression is reduced to its value.  Not located further: found while
fixing GCC-652, whose other shapes this does not share.

## Elsewhere

Clang: the mirror (`clang/test/Contracts/stmt-expr-local-generic-lambda.cpp`)
covers the trailing-return-type shape; a deduced-return mirror is owed.
