---
id: 1110-postcondition-pack-resubstitution
subject: "c++: contracts: don't re-substitute a postcondition that mentions a pack"
depends: []
regenerates: []
fixes: []
---

## Rationale

`rebuild_postconditions` re-substitutes a postcondition predicate so that the
result variable built for the definition replaces the one built for the
declaration.  For a predicate that mentions a function parameter pack that is
a second substitution of an already-substituted expansion, and the pack has no
argument mapping the second time round -- the elements are already there.  The
result was either a substitution failure or a silently wrong predicate.

`find_pack_use_r` and `contract_condition_uses_pack_p` answer the question
cheaply, and `rebuild_postconditions` skips the rebuild for such a
postcondition, whose result variable is remapped rather than re-substituted.

The pack-index case is the neighbouring one: a postcondition over
`Args...[N]` must check the selected element, not the whole expansion, which
is why the odr-use walk runs after substitution has resolved the index rather
than before.

## Compile gap

None: two new static helpers and one guard in an existing function.

## Contents

- gcc/cp/contracts.cc : @find_pack_use_r, @contract_condition_uses_pack_p, #116
- gcc/testsuite/g++.dg/contracts/cpp26/dcl.contract.func.p7-pack-index.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/dcl.contract.res.p1-pack.C : *
