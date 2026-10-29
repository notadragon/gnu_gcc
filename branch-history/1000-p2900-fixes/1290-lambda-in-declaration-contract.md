---
id: 1290-lambda-in-declaration-contract
subject: "c++: contracts: P2900: a lambda in the contract of a declaration"
depends: []
regenerates: []
fixes: [gcc-543, gcc-544]
---

## Rationale

A contract on a function declaration that is not a definition is parsed with
current_function_decl set to the function but no body and no cfun.  A lambda
in it broke two things.

GCC-543.  cp_parser_lambda_body saw current_function_decl and pushed the
function context, so push_function_context saved a placeholder cfun, and
pop_function_context left that placeholder behind -- with no language data,
and current_function_decl cleared -- for the next thing that tests cfun, a
default argument or the end of a template declaration, to crash on.  Both are
now restored as they were.

GCC-544.  In a template, pushtag records a local class with a DECL_EXPR for
instantiation, and the closure's context is the function; with no statement
list open, add_stmt crashed.  A closure in such a contract is instantiated
with the contract's LAMBDA_EXPR, so the DECL_EXPR is only added when a
statement list is open.

With no enclosing function body, `function_depth` is 0 while such a lambda
is parsed, so its `finish_function` could collect -- freeing the placeholder
function context, which only `function_context_stack` holds.  The depth is
raised for the duration (GCC-667, `contract-lambda-declaration-gc.C`).

## Compile gap

None.

## Contents

- gcc/cp/parser.cc : #21, #24
- gcc/cp/name-lookup.cc : #5
- gcc/testsuite/g++.dg/contracts/cpp26/decl-contract-lambda-default-arg.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-lambda-declaration-gc.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/decl-contract-lambda-default-arg-nsscope.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/redecl-unnamed-param-lambda-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/template-decl-contract-lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/template-decl-contract-lambda-class.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/template-decl-contract-lambda-nsscope.C : *
