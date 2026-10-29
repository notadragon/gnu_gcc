---
id: 1070-lambda-capture-constify
subject: 'c++: contracts: P2900: constify a by-reference lambda capture in a predicate'
depends: [1050-predicate-lambda-constify]
regenerates: []
fixes: []
---

## Rationale

The companion to `1050-predicate-lambda-constify`, for the other way a lambda
in a predicate can reach an entity declared outside it.  1050 handles naming
the entity directly in the lambda body; this commit handles *capturing* it.
[expr.prim.id.unqual]/3+d makes the entity const inside the contract
assertion, so a by-reference capture of it formed by a lambda that lexically
appears in the predicate has to be a reference to const -- otherwise the
lambda body gets a mutable alias to something the predicate is forbidden to
change.

`add_capture` const-qualifies the referent when the capture is by reference,
is not `this`, is formed inside a predicate, and
`capture_reaches_outside_predicate` says the initialiser ultimately designates
an entity declared outside the predicate.  That helper is the interesting
part and is what a reviewer should read closely: a `PARM_DECL` is outside
(including a postcondition's artificial result parameter); a local or a
capture proxy of a lambda that is itself in the predicate is not -- a by-value
capture there makes a fresh in-predicate copy and so cuts the chain, and pure
by-reference proxy chains within the predicate need no help, the normal
capture-type machinery propagates constness along them.  A capture proxy of
the lambda whose body holds the contract assertion is outside, whether it
captured by reference or by copy.

An array of runtime bound is captured as a pointer to its first element and
a bound, not as a reference, so for it `add_capture` const-qualifies the
array type before building that pointer.

A dependent capture cannot be decided at parse time: an implicit `[&]` in a
lambda inside a predicate in a templated function has a `DECLTYPE_TYPE`
referent.  `DECLTYPE_FOR_CONST_REF_CAPTURE` records the obligation on that
type and `tsubst` discharges it when the referent becomes known.
`cp_parser_lambda_expression` sets `LAMBDA_EXPR_IN_CONTRACT_PREDICATE_P`,
which every test above keys on.

## Compile gap

None: the flag, the `DECLTYPE` bit and both consumers arrive together.

## Contents

- gcc/cp/cp-tree.h : /DECLTYPE_FOR_CONST_REF_CAPTURE \(in DECLTYPE_TYPE\)/, @enum, @get_vec_init_expr
- gcc/cp/lambda.cc : @vla_capture_type, @capture_reaches_outside_predicate, /P2900: within a contract predicate an entity declared outside the/, /P2900: a VLA declared outside a contract predicate is const in it,/
- gcc/cp/parser.cc : #15
- gcc/cp/pt.cc : /a by-reference capture of an entity that is const within/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-lambda-capture-constify.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-array-ref-capture-constify.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-enclosing-lambda-capture-constify.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-constified-by-own-contract.C : *
