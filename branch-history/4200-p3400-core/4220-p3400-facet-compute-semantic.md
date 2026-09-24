---
id: 4220-p3400-facet-compute-semantic
subject: 'c++: contracts: P3400: the compute_semantic label facet'
depends:
  - 4200-p3400-core
regenerates: []
fixes: []
---

## Rationale

`semantic_computation_label`: the facet that lets a label transform the
evaluation semantic a contract assertion was configured with.

Three functions and a predicate:

* `compute_semantic_core` is the single probe -- call the label's
  `compute_semantic` with the incoming semantic, constant-evaluate the
  result, and report separately whether the answer landed inside the
  allowed set.  Both callers below go through it so that the probe itself
  is written once.

* `apply_compute_semantic` is the compile-time-resolved path.  A result
  outside the allowed set is an error there, because the semantic is being
  baked into the emitted check and there is no later moment at which to
  react.

* `apply_compute_semantic_value` is the P3595 dynamic-dispatch path.  The
  same out-of-range result cannot be an error there -- the input semantic
  is only known at run time -- so it yields `CES_INVALID`, the sentinel
  that the runtime selector turns into an enforced violation.  The two
  wrappers exist purely because that one decision differs.

* `label_has_compute_semantic` is a presence test with no evaluation, used
  by the P3595 machinery to decide whether an assertion needs the runtime
  selector at all.  It is deliberately cheaper and more permissive than the
  full probe: it asks only whether the member exists.

The facet has to be applied at parse time, before `contract_active_p`
decides whether to emit any code: a label that turns `ignore` into a
checking semantic must not have its assertion stripped first.

## Compile gap

`compute_semantic_core` calls `call_label_method`, and the error path names
`CES_INVALID` and the allowed-mask constants; all are in
`4200-p3400-core`.  The callers -- `ensure_evaluation_semantic` and the
P3595 selector construction -- are outside this entry, so with only the core
present these four functions are defined and unused, which builds.

The trap is silence.  With the framework present and this facet absent, a
label carrying `compute_semantic` does not fail to build and does not warn:
`resolve_contract_label` validates the control object, finds nothing it must
reject, and the assertion is emitted with the *configured* semantic
unchanged.  A `review` label -- whose whole content is a `compute_semantic`
downgrading `enforce` to `observe` -- silently keeps enforcing.  Nothing in
the front end can tell the difference between "this label has no
compute_semantic" and "compute_semantic was never wired up", which is why
`p3400-facet-review.C` is a run test and not a scan test.

## Contents

- gcc/cp/contracts.cc : @label_has_compute_semantic, /^compute_semantic_core \(tree label/, /^apply_compute_semantic \(tree label/, /^apply_compute_semantic_value \(tree label/
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-ignore.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-review.C : *
