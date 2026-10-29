---
id: 0230-module-template-contracts
subject: 'c++: contracts: stream contract specifiers through modules'
depends: []
regenerates: []
fixes: [gcc-553]
---

## Rationale

GCC-553.  A function template, or a member of a class template, exported
from a module kept its contracts in the module's own translation unit but
lost them in an importer that instantiated it: `trees_out::fn_parms` walked
`get_fn_contract_specifiers` only for the dependency graph and never wrote
it, and `trees_in::fn_parms_init` read only the pre/post function
references.  A non-template function was unaffected, because its checks are
already in the body streamed from the module.

The contract specifiers are now written with the parameters and set on the
declaration read; when that declaration merges with an existing one, it is
discarded and the existing declaration keeps its own.

## Compile gap

None.  `get_fn_contract_specifiers` and `set_fn_contract_specifiers` are
upstream's.

## Contents

- gcc/cp/module.cc : #0, #1
- gcc/testsuite/g++.dg/modules/contract-template-export_a.C : *
- gcc/testsuite/g++.dg/modules/contract-template-export_b.C : *
