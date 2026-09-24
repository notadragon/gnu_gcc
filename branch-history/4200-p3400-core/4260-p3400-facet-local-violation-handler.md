---
id: 4260-p3400-facet-local-violation-handler
subject: 'c++: contracts: P3400: the local_violation_label facet and its trampoline'
depends:
  - 4200-p3400-core
regenerates: []
fixes: []
---

## Rationale

`local_violation_label`: the facet by which a label supplies its own
violation handler, consulted before the global one and able to claim the
violation outright.

The compiler cannot call a label's member function from the runtime, so
`build_local_violation_trampoline` synthesises a bridge with the ABI's
erased signature -- `int (const void *label, const void *violation)` --
which casts both arguments back and calls
`label.handle_contract_violation(v)`.  A handler may return `void` or
`violation_handled`; the trampoline normalises that to the `int` the ABI
carries, returning `0` (not handled) for the void case.  One trampoline is
built per label *type*, not per assertion, and the key is
`TYPE_MAIN_VARIANT` -- and not the type tree as written, which
`p3400-facet-trampoline-shared.C` pins: substituting a dependent label
yields a distinct cv-variant of the same type per instantiation, and so a
fresh trampoline for each.

Two decisions in here are load-bearing and neither is obvious:

* The label is cast to `const LabelType *`.  The concept requires a const
  control object, so a non-const `handle_contract_violation` does not
  provide the facet and is reported by the near-miss warning long before a
  trampoline is built.  A non-const cast would be the single
  point at which this trampoline and the query trampoline disagree, and
  would imply, wrongly, that a non-const handler can reach here.
  `p3400-nonconst-handler-not-called.C` is the pin.

* `*resolved_fn_out` records the `FUNCTION_DECL` overload resolution
  actually picked, or `NULL_TREE` when that cannot be determined -- a
  virtual handler, for instance.  Nothing in this commit reads it; it exists
  for the rethrow analysis in `4270-p3400-bypass-rethrowing-local-handler`,
  which must reason about the concrete callee rather than the overload set
  and must decline to reason when it cannot have one.

The exception-path tests are the interesting behaviour: a handler that
returns without claiming leaves the violation to the global handler and the
predicate's own exception still has to reach the right place, and the order
in which a chain of handlers from a combined label runs is right-to-left.

## Compile gap

`begin_contract_trampoline`, `finish_contract_trampoline`,
`abandon_contract_trampoline` and the `trampoline_scope` sentry are not in
this entry -- they are shared trampoline machinery introduced by
`2600-source-location-machinery` -- and `lookup_std_contracts_type` and the
`local_violation_trampoline_map` / `local_violation_handler_fn_map` globals
are in `4200-p3400-core`.

The reverse gap is the one worth recording: `4200-p3400-core` *calls*
`build_local_violation_trampoline` from `resolve_contract_label`, so the
core does not build without this commit.  `resolve_contract_label` is a
single 361-line segment in the net diff and cannot be divided, so the call
cannot travel with the callee; ordering this commit after the core means the
core is the one that needs the forward declaration stubbed, not this one.

The semantic trap, if that stub were left as a no-op rather than filled in:
nothing fails.  `local_violation_trampoline_map` simply never acquires an
entry, `build_contract_data_block_ctor` sees no local handler to record, and
every violation goes straight to the global handler -- which is exactly what
a label without the facet is supposed to do.  A test that installs a global
handler as well as a local one therefore still passes; only a test that
asserts the local handler *ran*, or that it claimed the violation and
suppressed the global one, can see the difference.

## Contents

- gcc/cp/contracts.cc : @build_local_violation_trampoline, /bool can_be_const = true;/, /resolved_fn_out = cp_get_callee_fndecl_nofold/, /tree eval_semantic = CONTRACT_EVALUATION_SEMANTIC \(contract\);/, /tree int_result = build1 \(NOP_EXPR, integer_type_node, call\);/, /std_src_loc_impl_ptr = get_src_loc_impl_ptr/, /builtin_contract_violation_type, 7,/
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-combine-local-handler.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-local-chain.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-local-exception.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-local-handler.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-local-static.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-trampoline-shared.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-local-handler-exception-path-codegen.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-local-handler-exception-path.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-nonconst-handler-not-called.C : *
