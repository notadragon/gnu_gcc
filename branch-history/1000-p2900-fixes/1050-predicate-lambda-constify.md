---
id: 1050-predicate-lambda-constify
subject: 'c++: contracts: constify a variable named inside a lambda in a predicate'
depends: []
regenerates: []
fixes: [gcc-18]
---

## Rationale

GCC-18 (PR c++/127293).  [expr.prim.id.unqual]/3+d makes an entity declared
outside a contract-assertion `C` const when it is named inside `C`, and a
lambda written in the predicate is still inside `C`.  Upstream keyed the whole
rule on `processing_contract_condition`, which only says whether the
*innermost* binding level is the contract scope -- so it goes false the moment
a lambda in the predicate pushes its own scopes, and a namespace-scope
variable named in that lambda's body was not constified.  The paragraph's own
example is explicit about this case: given a namespace-scope `int n`,
`pre([=,&i,*this] mutable { ++n; ... }())` is an error.

`constify_from_lambda_in_predicate_p` answers the question the flag could not:
walk out from the current binding level looking for an `sk_contract` level,
stopping at namespace scope, and if one is found decide whether `DECL` was
declared between here and there.  `constify_in_contract_predicate_p` is the
combined test the callers now use, and is deliberately additive -- when the
innermost level *is* the contract scope nothing new runs and the existing path
is unchanged.

Two exemptions keep the rest of the standard's example working, and are what a
reviewer should check.  A lambda **capture proxy** is exempt: the
id-expression denotes a member of the closure type, which the example marks
OK, and a captured entity that must be const already is, because its capture
initialiser was constified in the enclosing predicate.  An entity **declared
inside** the predicate -- a local of the lambda -- is exempt because it is not
"declared outside of C".  Members reached through a captured `*this` never
arrive here at all: they are `FIELD_DECL`s on the member-access path.

Also check the unwrapping in `constify_from_lambda_in_predicate_p`: the caller
may pass a location wrapper or a dereference of a reference, and
`is_capture_proxy` asserts on the former.

## Compile gap

The two call sites this commit rewrites -- in `finish_id_expression_1` and in
`tsubst_expr` -- each did two things at once, and only one of them belongs
here.  Besides swapping `processing_contract_condition` for the new predicate,
they drop the neighbouring call to `check_param_in_postcondition`, the
upstream implementation of [dcl.contract.func]/7.  So between this commit and
`1060-postcondition-odr-use`, which reinstates that rule as a walk over the
finished predicate, the const requirement on a by-value parameter named in a
postcondition is not diagnosed at all.  The comment this commit adds at the
`finish_id_expression_1` site already points at
`check_postcondition_param_odr_uses`, which 1060 introduces.

Nothing else: the new predicate is exported through `contracts.h` here, and
both call sites are updated here.

## Contents

- gcc/cp/contracts.cc : #17
- gcc/cp/contracts.h : @constify_contract_access
- gcc/cp/pt.cc : @tsubst_expr
- gcc/cp/semantics.cc : #7
- gcc/testsuite/g++.dg/contracts/cpp26/contract-predicate-constify-lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-predicate-constify-storage.C : *
