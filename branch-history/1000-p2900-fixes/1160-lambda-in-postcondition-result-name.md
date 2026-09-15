---
id: 1160-lambda-in-postcondition-result-name
subject: 'c++: contracts: let a lambda appear in a postcondition with a result name'
depends: []
regenerates: []
fixes: [gcc-11]
---

## Rationale

GCC-11 (PR c++/127284).  A lambda written in a non-member postcondition that
has a result-name-introducer failed to parse.  Two independent obstacles, both
inherited from a carve-out written for requires-expressions.

`cp_parser_lambda_expression` clears `processing_template_decl` for a lambda
appearing directly in a requires-expression (the PR99546 carve-out).  A
postcondition raises `processing_template_decl` for the same reason a
requires-expression does -- its result variable is typed `auto` until the
return type is known -- so the carve-out has to extend to a contract scope, or
the lambda is parsed as a template pattern it is not.

The second obstacle is the consequence of the first.  A lambda parsed with
`processing_template_decl` cleared carries no `DECL_TEMPLATE_INFO`, so it is
already complete and concrete.  A requires-expression's operand is never
substituted afterwards, so this never came up; a postcondition predicate *is*,
by `rebuild_postconditions`, and `tsubst_lambda_expr` walked into
`tsubst_function_decl`, whose first act is to assert that nobody should be
tsubst'ing into a non-template function.  `tsubst_lambda_expr` now hands such
a lambda back unchanged.

A reviewer should check the guard's third conjunct: `DECL_LOCAL_DECL_P` is
excluded so that a block-scope function declaration is not caught by the same
test.

## Compile gap

None: two small guards, no new interfaces.

## Contents

- gcc/cp/parser.cc : #6
- gcc/cp/pt.cc : /A lambda whose operator\(\) carries no template info was parsed with/
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-in-postcondition-result-name.C : *
