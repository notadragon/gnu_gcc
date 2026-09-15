---
id: 1070-lambda-capture-constify
subject: 'c++: contracts: constify a by-reference lambda capture in a predicate'
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
(including a postcondition's artificial result parameter); a by-value capture
proxy is *not*, because a by-value capture makes a fresh in-predicate copy and
so cuts the chain; a local of a lambda that is itself in the predicate is not.
Pure by-reference proxy chains need no help -- the normal capture-type
machinery propagates constness along them.

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
- gcc/cp/lambda.cc : @vla_capture_type, @capture_reaches_outside_predicate, @add_capture
- gcc/cp/parser.cc : #5
- gcc/cp/pt.cc : @tsubst
- gcc/testsuite/g++.dg/contracts/cpp26/contract-lambda-capture-constify.C : *
