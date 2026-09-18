---
id: 1120-outlined-checks-by-reference
subject: "c++: contracts: make outlined checks see the guarded function's objects"
depends: []
regenerates: []
fixes: [gcc-15]
---

## Rationale

GCC-15 (PR c++/127289).  Under `-fcontract-checks-outlined` the predicates are
moved into separate `.pre`/`.post` functions, and `build_contract_condition_function`
copied the guarded function's by-value parameters -- and the postcondition
result -- **by value**.  A predicate that mutates one therefore mutated a copy:
the effect was invisible to the guarded function's body and to its caller,
while a mutation of shared state in the same predicate was still visible.
That partial application is not something the permission to elide a predicate
in [basic.contract.eval] allows; eliding produces all of a predicate's side
effects or none of them, never some.  Upstream's own testsuite records the
defect, with `expr.prim.id.unqual.p7-3.C` marked
`dg-xfail-run-if "PRXXXXXX"` -- a placeholder where a bug number belongs.

The checking function's parameters are now references when the checks are
outlined, and `build_arg_list` and `add_post_condition_fn_call` take the
addresses at the call site.  Two exclusions are load-bearing and are what a
reviewer should check.  Artificial parameters (`this`, `__in_chrg`,
`__vtt_parm`) are left alone: predicates do not name them and `this` is a
pointer already.  So are types with `TREE_ADDRESSABLE` set -- a class with a
non-trivial copy constructor or destructor is *already* passed by invisible
reference, so the two functions share the object without help, and adding a
reference on top produces a reference to the reference slot, which turns a
working case into a silently wrong one.

## Compile gap

The reading half of this fix is not here.  `remap_as_value` -- which maps a
use of a parameter inside a predicate to `*DP` rather than `DP` when the
outlined parameter is a reference -- and its callers in `remap_contract` and
`setup_param_remap` belong to `4000-p3098`, which owns those functions.
Neither half works alone: with only this commit the outlined predicate has
reference parameters and reads them as if they were values.  A commit standing
alone here would have to carry a local version of that remap for 4000 to
replace.

## Contents

- gcc/cp/contracts.cc : #29, #30, #31, #32, #33, #34, #35, #36, @build_arg_list, @add_post_condition_fn_call
- gcc/testsuite/g++.dg/contracts/cpp26/contract-outlined-observable-effects.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/expr.prim.id.unqual.p7-3.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/expr.prim.id.unqual.p7-4.C : *
