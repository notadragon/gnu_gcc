---
id: 4240-p3400-facet-identification
subject: 'c++: contracts: P3400: the identification (group_names) label facet'
depends:
  - 4200-p3400-core
regenerates: []
fixes: []
---

## Rationale

`identification_label`: the facet that associates a contract assertion with
one or more named groups, so that a configuration file can select semantics
by group rather than by source location.

The whole of the front-end side is `ensure_contract_groups`: probe the
label for a const, accessible `group_names`, constant-evaluate it, and
flatten the result into a `TREE_LIST` of `STRING_CST` group names hung off
the contract.  Two shapes have to be flattened, because the library offers
both -- a `CONSTRUCTOR` of `STRING_CST` (an array of pointers) and a
`CONSTRUCTOR` of `INTEGER_CST` characters (the `char[N][M]` that
`""group` and `assertion_group_label` actually produce).  Empty rows are
skipped: a combined label's `group_names` is sized as the *sum* of its
constituents' counts, and de-duplication leaves the tail zero-filled.

It is lazy on purpose.  The names are wanted only when the configuration is
consulted or a violation is reported, and computing them eagerly at
`grok_contract` time would constant-evaluate a member of every label on
every assertion.  `error_mark_node` is the memoized "this label has no
groups" answer, not a failure -- which is why the absent case is
indistinguishable from the malformed case and neither is diagnosed.

`p3400-facet-group-const.C` pins the const requirement.  D3400R5 wants
`group_names` const so that nothing suggests a label's group membership
could change at run time and have an effect: nothing reads it after
translation.

## Compile gap

None: `label_facet_accessible_p`, `CONTRACT_GROUPS` and the lazy-resolution
call site are all in `4200-p3400-core`.

The trap is the memoized sentinel.  With the framework present and this
facet absent, `CONTRACT_GROUPS` is simply never populated, and every
consumer treats that exactly as it treats a label with no `group_names` at
all: the assertion compiles, the configuration file's group entries match
nothing, and the violation is reported with an empty group list.  No
diagnostic is possible, because "not a member of any group" is a legal and
common state.  A group-keyed configuration therefore silently degrades to
the default semantic for every assertion -- which is why
`p3400-group-basic.C` and friends assert on the semantic that actually ran
rather than on a diagnostic.

## Contents

- gcc/cp/contracts.cc : @ensure_contract_groups, /get_identifier \("group_names"\)/
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-group-const.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-basic.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-combined.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-multi.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-postcondition.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-with-facets.C : *
