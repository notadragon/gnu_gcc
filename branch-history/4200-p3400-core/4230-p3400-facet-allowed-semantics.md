---
id: 4230-p3400-facet-allowed-semantics
subject: 'c++: contracts: P3400: the allowed_semantics label facet'
depends:
  - 4200-p3400-core
  - 4220-p3400-facet-compute-semantic
  - 3000-p3097
  - 0430-refinable-body-contracts
regenerates: []
fixes: []
---

## Rationale

`allowed_semantics_label`: the facet by which a label narrows the set of
evaluation semantics its assertions may be given.

Two halves meet, and only one of them is separable.  The *label's* half --
probing `label.allowed_semantics.contains(sem)` once per semantic and
storing the intersection in `CONTRACT_ALLOWED_MASK` -- sits inside
`resolve_contract_label`, which the net diff files as one 361-line addition
and which therefore cannot be lifted out (see `4200-p3400-core`).  What is
separable is the *base* the label intersects with:
`contract_base_allowed_mask`, the flag-gated full set, which adds `assume`
under `-fcontracts-allow-assume` and the two noexcept semantics under
`-fcontracts-p4298`.

That base is the reason the facet needs a commit of its own even though it
is eleven lines.  There are two allowed-set computations on this branch and
they deliberately differ: `resolve_contract_label` starts from
`CES_ALL_ALLOWED_WITH_EXTENSIONS`, the flag-*independent* full set, so a
label that explicitly allows `noexcept_enforce` is not stripped of it before
the flags are consulted; `contract_base_allowed_mask` applies the flag
gates, and runs later, at query construction.  Getting those two the same
way round is the whole content of `p3400-allowed-mask-no-noexcept.C`.

The tests pin the two distinct out-of-range behaviours, which are easy to
conflate and are not the same rule: a *configured* semantic outside the
allowed set is adjusted to the nearest allowed one, which is
implementation-defined and fine; a `compute_semantic` *result* outside the
allowed set is ill-formed and diagnosed (`p3400-facet-allowed-error.C`).
`p3400-facet-allowed-const.C` pins the const requirement -- a non-const
`allowed_semantics` is not a facet at all, and honouring it anyway makes a
bare label and its combined form disagree.

The allowed set also decides the refinable-body mark.  A function holding a
contract evaluation is a refinable body because another translation unit's
definition may evaluate it with another semantic; but the label's
allowed set is part of the declaration, the same in every unit, so when
it settles the evaluation in both directions the function need not be
marked (`contract_evaluation_settled_p`).  Direction A: every allowed
semantic ends the program on a violation or makes it undefined (enforce,
noexcept_enforce, quick_enforce, assume), so facts drawn from one
definition's evaluation hold for every other.  Direction B: no allowed
semantic lets an exception leave the function unless every one does --
the set is a subset of {noexcept_enforce, quick_enforce, assume}, or
{enforce} alone, or the function is noexcept.  A label that does not
restrict the set, a dynamically selected semantic and a caller-side
evaluation (where ignore is always allowed) are never settled.
`cp_genericize_r` and `finish_function` skip the mark for a settled
evaluation and for specifiers that are all settled, and a call of a P3097
wrapper whose evaluations are all settled does not mark its caller
(`contract_specifiers_settled_p`, `contract_wrapper_settled_p`).
`refinable-body-label.C` shows the callers of settled functions folding a
comparison and dropping a landing pad, and those of unsettled ones keeping
them.

## Compile gap

None for the code: `contract_base_allowed_mask` reads only `CES_*` and the
two flag variables, all in `4200-p3400-core`, and its callers are elsewhere.

The trap is that the fallback is not "no restriction" but "wrong
restriction, no diagnostic".  A mask is a bitmask with a permissive default,
so an unwired facet leaves every semantic allowed and every assertion
compiles; a *narrower*-than-intended base is equally quiet in the other
direction, because a configured semantic outside the set is silently
adjusted rather than rejected.  Both failure modes are invisible except by
observing which semantic actually ran, which is why every test here is a run
test except the two that are specifically about diagnostics.

## Contents

- gcc/cp/contracts.cc : /^contract_base_allowed_mask \(\)/, /^\/\* Reconstruct a base, callee-side, non-constant-evaluation contract_query$/
- gcc/cp/contracts.cc : @finish_contract_control_expr:1, /^contract_evaluation_settled_p \(tree contract, tree fndecl\)$/, /^contract_specifiers_settled_p \(tree fndecl\)$/, /^contract_wrapper_settled_p \(tree wrapper\)$/
- gcc/cp/contracts.h : /^extern bool contract_(evaluation|specifiers|wrapper)_settled_p/
- gcc/cp/cp-gimplify.cc : #6:1, #11:1
- gcc/cp/decl.cc : #18:1
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-label.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-label.json : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-allowed-mask-no-noexcept.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed-const.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed-error.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed-noexcept.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-nondependent-template.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-stateful-allowed.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/facet-diagnostic-text.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-scalar-allowed-semantics.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/evaluation-semantic-set-unspecified.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-allowed-semantics-conversion.C : *
