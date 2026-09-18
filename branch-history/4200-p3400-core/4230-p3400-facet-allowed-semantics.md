---
id: 4230-p3400-facet-allowed-semantics
subject: 'c++: contracts: the allowed_semantics label facet'
depends:
  - 4200-p3400-core
  - 4220-p3400-facet-compute-semantic
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

- gcc/cp/contracts.cc : /^contract_base_allowed_mask \(\)/
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-allowed-mask-no-noexcept.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed-const.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed-error.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed-noexcept.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-allowed.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-stateful-allowed.C : *
