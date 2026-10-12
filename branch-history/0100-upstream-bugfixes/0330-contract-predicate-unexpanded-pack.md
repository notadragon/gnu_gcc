---
id: 0330-contract-predicate-unexpanded-pack
subject: 'c++: contracts: diagnose an unexpanded parameter pack in a contract predicate'
depends: []
regenerates: []
fixes: [gcc-651]
---

## Rationale

GCC-651.  A function parameter pack named unexpanded in a contract predicate
-- `pre (xs > 0)`, `contract_assert (xs > 0)` -- was not diagnosed in the
template.  An instantiation reported "sorry, unimplemented: use of
'type_pack_expansion' in template", or nothing at all for a
`contract_assert`.  Every other expression context runs
`check_for_bare_parameter_packs`; a contract predicate never did.
`finish_contract_condition`, which every parsed predicate passes through on
its way to bool, now does, and gives up on the predicate if it finds one.
After substitution there is nothing left to find.

## Compile gap

None.

## Contents

- gcc/cp/contracts.cc : /A parameter pack named in the predicate must be expanded within it/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-unexpanded-pack.C : *
