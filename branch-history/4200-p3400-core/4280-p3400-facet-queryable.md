---
id: 4280-p3400-facet-queryable
subject: 'c++: contracts: the queryable_label facet and its trampoline'
depends:
  - 4200-p3400-core
  - 4260-p3400-facet-local-violation-handler
regenerates: []
fixes: []
---

## Rationale

`queryable_label`: the facet that lets a violation handler reach back
through an erased `contract_violation` and ask the label that produced it
for a value, keyed by an address the handler and the label agree on.

It is the same trampoline pattern as the local handler, one signature over:
`build_query_trampoline` synthesises `void *(const void *label, const void
*key, size_t index)` -- the ABI's `__cxa_query_fn_t` -- casts the label back
to `const LabelType *` and calls `label.query(key, index)`.  As with the
handler, one trampoline per label type, cached on `TYPE_MAIN_VARIANT`.  The
`index` parameter is what makes the facet usable on a combined label: the
library's `__combined_label::query` dispatches on it to reach the right
constituent.

The other half is the consumer:
`contract_violation::query_control_object` walks the
`__cxa_contract_data_block` chain for `CXA_FIELD_QUERY_FUNCTION` and
`CXA_FIELD_LABEL_PTR` and calls through, returning `nullptr` when either is
absent.  That is the whole runtime cost of the facet, and it is the evidence
for the ABI claim this branch makes elsewhere: `queryable_label` arrives
thousands of band numbers after the ABI it reads through, and costs two new
field ids and a data-block variant and nothing else -- no change to any
existing handler, and no version bump.

`p3400-facet-trampoline-in-body.C` and `p3400-facet-trampoline-scope.C` are
here rather than with the handler because each exercises both trampoline
builders: a label is grokked in every context one can be grokked in --
mid-function-body, in a class, in a template -- and both builders must
survive being run with somebody else's parse state live.  What they pin is
that the builders save the class scope as well as `cfun`; saving `cfun`
alone ICEs in `poplevel_class`.

## Compile gap

`build_query_trampoline` uses the shared trampoline machinery
(`begin_contract_trampoline`, `abandon_contract_trampoline`,
`finish_contract_trampoline`, `trampoline_scope`) from
`2600-source-location-machinery`, and `query_trampoline_map` is in
`4200-p3400-core`.  `query_control_object` needs
`__cxa_find_field_value` and the two field ids from `2200-libcontracts`, and
its declaration in `<contracts>` is in `4200-p3400-core`.

As with the local handler, the forward reference runs the other way:
`resolve_contract_label`, in `4200-p3400-core`, calls
`build_query_trampoline`, and that call cannot be moved here because
`resolve_contract_label` is one indivisible segment.

The trap is that this facet fails *quietly and plausibly*.  With the
framework present and this commit absent, `query_trampoline_map` stays
empty, no query function reaches the data block, and
`query_control_object` -- if it is even there -- returns `nullptr`.  But
`nullptr` is also the legitimate answer for a label with no `query`, for a
key the label does not recognise, and for an unlabelled assertion; a handler
written to cope with those cannot distinguish "not queryable" from "query
was never wired up".  `p3400-facet-query-no-label.C` deliberately pins the
legitimate `nullptr`, which is precisely why the other three tests must
assert on a *non-null* answer with a known value.

## Contents

- gcc/cp/contracts.cc : /__contract_query_N/, /^build_query_trampoline \(tree label_type\)/
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-query-basic.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-query-combined.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-query-no-label.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-query-with-handler.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-trampoline-in-body.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-trampoline-scope.C : *
- libstdc++-v3/src/c++26/contract26.cc : @query_control_object
- libstdc++-v3/testsuite/18_support/contracts/query_control_object.cc : *
