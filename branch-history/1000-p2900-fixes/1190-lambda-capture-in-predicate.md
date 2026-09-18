---
id: 1190-lambda-capture-in-predicate
subject: 'c++: contracts: let a lambda in a contract predicate capture a parameter'
depends: [1070-lambda-capture-constify]
regenerates: []
fixes: [gcc-31]
---

## Rationale

GCC-31 (PR c++/117435).  A contract-assertion scope is a capture-transparent
scope: [expr.prim.lambda.capture]/3.3 permits a capture-default or
simple-capture in a lambda whose innermost enclosing scope is one, so naming
the enclosing function's parameter in that lambda's body is a **capture**.  GCC
did not treat it that way, and the defect has two layers -- the second only
observable once the first is fixed, which is why they are one commit and one
report.

Layer 1: the capture is rejected outright.  `void f (int x) pre ([x] { return
x > 0; } ())` gave "use of parameter outside function body".  `outer_var_p`
had not routed the name into the capture machinery because it requires
`DECL_FUNCTION_SCOPE_P`, and on a free function the predicate is parsed off
the declarator before any `FUNCTION_DECL` exists, so the parameter's
`DECL_CONTEXT` is still null.  (In a member function, or a lambda's own
contract, the context is set by the time the predicate is parsed, which is why
those cases always worked.)  `finish_id_expression_1` now enters
`process_outer_var_ref` directly for a lambda marked as being in a predicate;
that function copes with the null context, because the enclosing non-lambda
function context is null too and the two therefore match.

Layer 2, which PR117435 actually reports as its primary symptom: once the
capture is formed, a `contract_assert` nested inside the predicate lambda ICEs
in `expand_expr_real_1`.  `processing_contract_condition` is set again for
that nested assertion, and it exempts a name from the capture machinery --
correctly for a predicate written *directly* on a function, where the contract
is part of that function's declaration and there is nothing in between, but
not for a lambda in the predicate, which is another function.  Skipping the
machinery left a bare reference to the enclosing function's parameter in the
lambda body, which survives to expansion and trips "Variables inherited from
containing functions should have been lowered by this point".  The exemption
is now tested only *after* the in-predicate-lambda case has been taken, which
is the whole of the fix and the order a reviewer should check.

## Compile gap

None.  `LAMBDA_EXPR_IN_CONTRACT_PREDICATE_P`, which both layers test, is
introduced by the dependency.

## Contents

- gcc/cp/semantics.cc : #1, #2
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-in-contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-in-contract-error.C : *
