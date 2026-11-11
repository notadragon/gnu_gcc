---
id: 0280-generic-lambda-pre-capture
subject: 'c++: contracts: a generic lambda''s contract predicate naming a capture'
depends: []
regenerates: []
fixes: [gcc-633]
---

## Rationale

GCC-633.  A generic lambda whose precondition named one of its captures ICEd
in `tsubst_expr`, with or without a trailing return type.

A specialization of a generic lambda's `operator()` has its contracts
substituted by `regenerate_decl_from_template`, before `instantiate_body`
substitutes the body that declares the specialization's own capture proxies.
The pattern's proxy therefore had no local specialization, and fell through to
the recovery for "a variable used in a late-specified return type", whose
assertion that the use is unevaluated does not hold for a predicate.

A proxy only stands for the closure member its `DECL_VALUE_EXPR` names, so in a
contract predicate an unspecialized proxy is now substituted by that value
expression: the member, reached through the specialization's object parameter,
which `tsubst_contract_specifier` has mapped.  The predicate then needs no
proxy in scope at all.

## Compile gap

None.  Everything named is upstream's.

## Contents

- gcc/cp/pt.cc : /Or for a capture named in the contract predicate of a/
- gcc/testsuite/g++.dg/contracts/cpp26/generic-lambda-pre-capture.C : *
