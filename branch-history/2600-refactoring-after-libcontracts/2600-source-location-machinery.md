---
id: 2600-source-location-machinery
subject: 'c++: contracts: retire the layout-compatible violation and source-location machinery'
depends:
  - 2200-libcontracts
regenerates: []
fixes: [gcc-37, gcc-38]
---

## Rationale

The base P2900 implementation carried its own hand-built, layout-compatible
copies of two library types: `__builtin_contract_violation_type`, matching
`std::contracts::contract_violation` field for field, and an `__impl` record
matching `std::source_location`'s.  Every violation object was a static
instance of the first holding a pointer to a static instance of the second,
and the two had to be kept byte-identical with the library by hand.

The data-block ABI added in `2200-libcontracts` needs neither: a data block
carries file, function, line and column as plain fields, and the runtime
reaches them through a descriptor table rather than through an agreed struct
layout.  So this commit deletes the machinery -- `get_contract_violation_fields`,
`init_builtin_contract_violation_type`, `get_contracts_source_location_impl_type`,
`get_src_loc_impl_ptr`, `build_contract_violation_constant`, and the
`lookup_std_type` helper that existed to inject `std::source_location` when the
header was absent.

What lands in the same lines of the net diff is the generic trampoline shell
that later work builds on: `trampoline_scope` (a `push_to_top_level` sentry
that makes it safe to define an artificial function in the middle of parsing
something else), `begin_contract_trampoline`, `finish_contract_trampoline` and
`abandon_contract_trampoline`.  Nothing calls them yet; `4200-p3400-core`
supplies both callers.  They are here rather than there because the histogram
diff aligned them onto the deleted functions -- the same anchors, and in three
cases the same function-header lines, carry the deletion and the replacement.

Behaviour-neutral apart from the renames of four function headers under the
`@static` anchor, one of which belongs to the `handle_contract_violation`-alias
rework whose body is in `2200-libcontracts`.  A one-line signature does not
read sensibly on its own, and the net diff offers no way to separate it from
the three renames beside it.

## Compile gap

The trampoline shell is unreferenced until `4200-p3400-core`, so a compiler
built here warns about unused static functions.  `contracts_tu_local_named_var`
loses its only pre-existing caller in the same commit.

## Contents

- gcc/cp/contracts.cc : @abandon_contract_trampoline, @contracts_tu_local_named_var, @get_contracts_source_location_impl_type, @get_src_loc_impl_ptr, @init_contracts, @retain_decl, /^get_or_create_contract_wrapper_function \(tree fndecl, tree tuple\)$/, /^maybe_emit_hcv_alias \(\)$/, /^begin_contract_trampoline \(const char \*name_prefix, int &counter,$/, /^finish_contract_trampoline \(tree body, tree compound_stmt\)$/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-source-location-alias.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-source-location-union.C : *
