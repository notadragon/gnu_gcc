---
id: 0240-contract-explicit-copy-capture
subject: 'c++: an explicit by-copy capture in a contract predicate is not const'
depends: []
regenerates: []
fixes: [gcc-606]
---

## Rationale

GCC-606.  In a contract predicate an entity declared outside it is named as
const ([expr.prim.id.unqual]), but that does not change the entity's type: a
by-copy capture of it is a closure member of the entity's own type, which a
mutable lambda may modify -- the paragraph's own example does it with [=].
GCC got [=] and an init-capture right, but computed the field type of an
explicit simple capture ([v]) from the constified id-expression, so
`[v]() mutable { return ++v; }` was rejected.

`lambda_capture_field_type` now strips the contract const wrapper from the
initializer of a by-copy capture without an initializer before taking its
type.  A by-reference capture names the entity and stays const.

## Compile gap

None.  `contract_const_wrapper_p` and `strip_contract_const_wrapper` are
upstream's (`contracts.h`), and `lambda.cc` already includes it.

## Contents

- gcc/cp/lambda.cc : /In a contract predicate an entity declared outside it is named as/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-explicit-copy-capture.C : *
