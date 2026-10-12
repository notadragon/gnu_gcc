---
id: 0340-pch-contract-tables
subject: 'c++: contracts: hash the contract tables so that a PCH preserves them'
depends: []
regenerates: []
fixes: [gcc-646, gcc-647]
---

## Rationale

GCC-646 and GCC-647.  `contract_decl_map`, which holds every function's
contract specifiers, hashed its keys by address.  Writing a precompiled
header moves every tree and does not rehash the table, so after a PCH was
read back a lookup found a function's entry only by chance: a function
template declared in the header and instantiated after it lost its contract
assertions (in constant evaluation and at run time alike), and a
redeclaration after the header was checked as if the header's declarations
had none ("declaration adds contracts").  Which functions were affected
depended on the particular `.gch`.

`contract_pch_hash` hashes a declaration or a type by its uid and an
identifier by its spelling -- what `decl_tree_map` does for the same reason
-- and falls back to the address for anything else.  `contract_decl_map`
and the other upstream contracts tables keyed by declarations
(`decl_pre_fn`, `decl_post_fn`, `orig_from_outlined`, `decl_for_wrapper`)
use it; the tables later commits add or rewrite use it too.

The regression tests instantiate twelve templates after a PCH and redeclare
twelve functions of each shape, so that a regression cannot pass by chance.

## Compile gap

None.

## Contents

- gcc/cp/contracts.cc : /Hash a key of the maps below by something a precompiled header preserves/, /^struct contract_pch_hash : ggc_ptr_hash <tree_node>$/, /^struct contract_decl_traits$/, /^struct pch_traits$/, /^  \*contract_decl_map;$/, /^static GTY\(\(\)\) hash_map<tree, tree, pch_traits> \*decl_pre_fn;$/, /^static GTY\(\(\)\) hash_map<tree, tree, pch_traits> \*orig_from_outlined;$/, /^static GTY\(\(\)\) hash_map<tree, tree, pch_traits> \*decl_for_wrapper = nullptr;$/
- gcc/testsuite/g++.dg/pch/contract-template-after-pch.C : *
- gcc/testsuite/g++.dg/pch/contract-template-after-pch.Hs : *
- gcc/testsuite/g++.dg/pch/contract-redecl-after-pch.C : *
- gcc/testsuite/g++.dg/pch/contract-redecl-after-pch.Hs : *
