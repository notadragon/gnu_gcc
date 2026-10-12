---
id: 0200-block-scope-first-decl-contract
subject: 'c++: contracts: keep the contract of a block-scope first declaration'
depends: []
regenerates: []
fixes: [gcc-551]
---

## Rationale

GCC-551.  A block-scope function declaration can be the first declaration
of the function, and its function-contract-specifiers are then the
function's ([dcl.contract.func]).  `push_local_extern_decl_alias` creates
the namespace-scope alias that later declarations merge with by
`copy_decl`, which does not carry contract specifiers (they live in a side
table keyed by the declaration).  A definition that omitted the contract
therefore lost it, and one that repeated it was rejected as "declaration
adds contracts".

The alias now receives the block-scope declaration's contracts, remapped
onto the alias's own parameters with `copy_and_remap_contracts`.  Only a
newly created alias does: when a namespace-scope declaration already
exists, it is the first declaration and its contracts govern.

## Compile gap

None.  `copy_and_remap_contracts` and `set_fn_contract_specifiers` are
upstream's, declared in `contracts.h`.

## Contents

- gcc/cp/name-lookup.cc : /#include "contracts\.h"/, /This is the first declaration of the function, so its/
- gcc/testsuite/g++.dg/contracts/cpp26/block-scope-decl-then-def-contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/block-scope-first-decl-contract.C : *
