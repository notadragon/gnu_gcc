---
id: 0160-requires-clause-on-parameter
subject: 'c++: diagnose a requires-clause on a parameter of function type'
depends: []
regenerates: []
fixes: [gcc-47]
---

## Rationale

GCC-47, and not a contracts defect -- it is in this band because it is
upstream's and this branch happens to fix it.

A parameter declared with a function declarator is adjusted to a pointer to
function ([dcl.fct]/5), so it declares no function and a requires-clause on
it constrains nothing.  It was accepted and silently dropped.

The check that should have caught it already exists.  `grokdeclarator`
refuses a misplaced requires-clause at four points, and the fourth --

    if (!FUNC_OR_METHOD_TYPE_P (type))
      ... error_at (..., "requires-clause on declaration of non-function type")

-- asks exactly the right question and never fires for a parameter, because
the function-to-pointer adjustment happens LATER in the same function than
the guard that depends on it.  At that point the parameter still looks like a
`FUNCTION_TYPE`.  So this asks about `decl_context` instead, which is settled
whatever the decay has done.

It sits beside `1250-contract-on-non-function-declarator`, which closes the
identical hole for a C++26 function-contract-specifier: the two travel in the
same declarator slot and are diagnosed at the same four points, which is how
one was found while fixing the other.  The two are separate commits because
only one of them is about contracts, and this one is reportable upstream on
its own.

The classification is not written twice.
`1250-contract-on-non-function-declarator` introduces
`classify_non_function_declarator`, which answers the question once for both
specifiers; this commit is its second caller.  Only the parameter position is
acted on here -- the other three already have requires-clause diagnostics,
and their wording is upstream's and deliberately left alone.

## Compile gap

Needs `classify_non_function_declarator`, which
`1250-contract-on-non-function-declarator` adds and which precedes this
commit.  Otherwise none: one `error_at` in `grokdeclarator`, and a test that
needs only C++20.

## Contents

- gcc/cp/decl.cc : /What kind of thing a declarator declares/, @non_function_declarator, #7:0, #7:2
- gcc/testsuite/g++.dg/concepts/requires-clause-on-parameter.C : *
