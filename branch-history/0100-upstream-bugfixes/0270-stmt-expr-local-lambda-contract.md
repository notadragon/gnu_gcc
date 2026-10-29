---
id: 0270-stmt-expr-local-lambda-contract
subject: 'c++: contracts: a statement expression''s variable in a lambda''s contract predicate'
depends: []
regenerates: []
fixes: [gcc-632]
---

## Rationale

GCC-632.  A variable declared in a GNU statement expression in a lambda's
precondition or postcondition made the gimplifier assert in
`gimplify_var_or_parm_decl`, which catches a declaration it has not seen in
any `BIND_EXPR`.

Each emission of a predicate is a copy made by `copy_contracts_list`, whose
`copy_decl` hook kept every declaration as it was -- right for the
parameters, the result binding and the captures, which are declared
elsewhere.  But `remap_decls` rebuilds a copied `BIND_EXPR`'s variable list
only from the declarations it newly made, so a kept one dropped out of its
own statement expression.  The hook now copies a variable that a
`BIND_EXPR` inside the predicate declares, and keeps the rest.

A lambda with a deduced return type has its postcondition parsed as if in a
template while the return type is undeduced, and substituted once it is
known (`rebuild_postconditions`, with no template arguments).  There are no
template parameters then, and three things assumed there were:
`start_decl` handed a local to `push_template_decl`, which failed silently
and lost the declaration; `decl_dependent_p` did not count the local as
dependent, so a use of an `auto` one was "before deduction"; and `tsubst`
asserted on that `auto` with an empty argument vector, where it now leaves
it for the initializer to deduce.

A generic lambda is not covered: its specialization's contracts are
substituted before its function exists (GCC-652).

## Compile gap

None.  Everything named is upstream's; the tests use only upstream contracts.

## Contents

- gcc/cp/contracts.cc : /struct predicate_copy_data : copy_body_data/, /^      hash_set<tree> locals;$/, /auto collect_locals = \[\] \(tree \*tp, int \*, void \*data\) -> tree/
- gcc/cp/decl.cc : /A local declared in a statement expression in the predicate of a/
- gcc/cp/decl2.cc : /So does a local declared in a statement expression in the predicate of a/
- gcc/cp/pt.cc : /rebuild_postconditions substitutes a predicate parsed as if in a/
- gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-lambda-pre.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-lambda-deduced-post.C : *
