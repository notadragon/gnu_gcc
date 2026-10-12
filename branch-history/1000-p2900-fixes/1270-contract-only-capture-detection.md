---
id: 1270-contract-only-capture-detection
subject: "c++: contracts: P2900: detect captures made only by contract assertions"
depends: []
regenerates: []
fixes: [gcc-495, gcc-503, gcc-670]
---

## Rationale

[expr.prim.lambda.closure]/10: a local entity implicitly captured by a lambda
only because contract assertions name it makes the program ill-formed.  Three
shapes escaped the check or crashed it.

GCC-495.  A VLA's capture proxy rebuilds the array from the {pointer, bound}
capture struct, so its DECL_VALUE_EXPR is not the COMPONENT_REF of the capture
field that set_contract_capture_flag asserted it was; it crashed.  The
function now looks through that shape (and a *this capture's ADDR_EXPR) for
the member of the closure.

GCC-670.  Name lookup finds a VLA's capture proxy itself rather than the VLA,
so a use outside a contract assertion that follows the one inside it never
reached process_outer_var_ref, which is where such a use clears the mark, and
the capture was diagnosed although the lambda uses the VLA outside contract
assertions too; likewise from a lambda nested in that one.
finish_id_expression clears the mark on a use of a VLA's capture proxy
outside a predicate, unless a lambda in a contract predicate lies in between.

GCC-503.  An implicit capture of `this' in a contract was never flagged:
lambda_expr_this_capture now marks it as process_outer_var_ref marks a
variable, and clears the mark on a use outside a contract, which
set_contract_capture_flag (now exported, and accepting the capture field) makes
possible.  And a generic lambda whose contract-only capture was diagnosed
crashed when instantiated: the instantiation has no such capture, and a copy
of the pattern's proxy has no context to be a proxy in, so after an error the
substitution gives up on it instead.

## Compile gap

None.

## Contents

- gcc/cp/semantics.cc : #1, #2, #3, #4, #5, #9
- gcc/cp/cp-tree.h : /^extern void set_contract_capture_flag/
- gcc/cp/lambda.cc : #7, #8, #9
- gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-vla-implicit-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-generic.C : *
