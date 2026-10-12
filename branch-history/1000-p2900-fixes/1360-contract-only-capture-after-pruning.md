---
id: 1360-contract-only-capture-after-pruning
subject: "c++: contracts: P2900: check contract-only captures once a lambda is complete"
depends: []
regenerates: []
fixes: [gcc-14, gcc-648, gcc-649]
---

## Rationale

[expr.prim.lambda.closure]/10 makes it ill-formed for a lambda to capture an
entity implicitly only because contract assertions of the lambda odr-use it.
The check ran at the end of `cp_parser_lambda_body', which was wrong twice.

GCC-649.  Naming a constant variable without odr-using it is no capture at
all.  The capture made provisionally when the name is seen is dropped by
`prune_lambda_captures' once the lambda is complete, and the check ran before
that, so `[=] { contract_assert (n > 0); }' for a `const int n' was rejected.

GCC-648.  A parameter pack is captured only when its elements are known, at
instantiation, which the parser's check never sees, so a pack named only in a
contract assertion was captured silently.

The check is now `check_contract_only_captures', called from
`finish_lambda_function' after the pruning, which the instantiation of a
lambda in a template reaches too.  A capture diagnosed in the template has
its flag cleared, so the instantiation's copy is not reported again.  A
lambda in a template is never pruned, not even (in practice) in the
instantiation, so there a capture of a constant cannot be told from an
odr-use and is not diagnosed (GCC-653, whose watch test this adds).

The diagnostic itself moves with the check, unchanged.  It takes the note's
location from the variable actually captured, never from the capture's
initializer unless that is a declaration: the initializer reads through the
enclosing closure for a capture made two lambdas out, and is a
{pointer, bound} constructor for a VLA (GCC-14, PR c++/126041).

The instantiation of a lambda in a template keeps its captures of constants
unpruned, but has folded away the uses that are not odr-uses, so a
contract-only capture of a constant is diagnosed there when a use of its
proxy remains (`lambda-contract-only-capture-constant-template.C`).

## Compile gap

None.

## Contents

- gcc/cp/lambda.cc : /Diagnose the captures of the current lambda that were made only by/, @check_contract_only_captures, /^    check_contract_only_captures \(current_lambda_expr \(\)\);$/
- gcc/cp/parser.cc : /Captures made only by contract assertions/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-capture-note-location.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-pack.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-constant.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-constant-template.C : *
