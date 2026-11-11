---
id: 1350-contract-this-pointer-to-const
subject: "c++: contracts: P2900: `this' is a pointer to const in a contract predicate"
depends: []
regenerates: []
fixes: [gcc-641]
---

## Rationale

GCC-641.  [expr.prim.this] combines const with the type of `this' where it
appears within a contract assertion, the bodies of lambdas in it included.
GCC constified only the object reached through `current_class_ref', so a
member access in a predicate was const but `this' itself was a plain pointer:
`[this] { return ++m > 0; } ()' and `[&] { return ++m > 0; } ()' in a
predicate modified the object, `sf (this)' passed it to an `S *', and a
`contract_assert' in a lambda's body could modify the enclosing object
through the lambda's `this'.

`finish_this_expr' now gives `this' in a predicate the type pointer to const,
and so does `maybe_resolve_dummy' for an implicit member access through a
lambda's `this' there (`maybe_dummy_object' agrees on the qualifiers).  A
lambda in a predicate that captures `this' from outside it captures a pointer
to const (`add_capture').  A lambda's copy of *this stays the closure's own
member, which a mutable lambda may modify, as [expr.prim.id.unqual]'s example
shows.

`tsubst_lambda_expr' now copies `LAMBDA_EXPR_IN_CONTRACT_PREDICATE_P', so a
capture first made at instantiation -- of `this', or of a parameter pack's
elements -- is const-qualified as one made in the template is.

## Compile gap

None.  `LAMBDA_EXPR_IN_CONTRACT_PREDICATE_P' comes from
`1070-lambda-capture-constify', earlier in this band.

## Contents

- gcc/cp/contracts.cc : /where \[expr.prim.this\] combines const with the cv-qualifiers of its type/, @contract_this_is_const_p, @contract_const_this
- gcc/cp/contracts.h : /^extern tree contract_const_this/
- gcc/cp/lambda.cc : /A copy of \*this made by a lambda in a contract predicate is the/, /A lambda in a contract predicate that captures .this. from/, /An implicit member access in a contract predicate reaches the/
- gcc/cp/semantics.cc : /Within a contract predicate .this. is a pointer to const/
- gcc/cp/tree.cc : /^#include "contracts.h"$/, /As maybe_resolve_dummy will make it in a contract/
- gcc/cp/pt.cc : /Captures added at instantiation -- of a parameter pack, or of/
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-this-modifies-object.C : *
