---
id: 1040-pack-reference-constify
subject: 'c++: contracts: P2900: do not const-qualify a reference parameter pack'
depends: []
regenerates: []
fixes: [gcc-22]
---

## Rationale

GCC-22 (PR c++/126878, and PR c++/126039 for the `auto&&...` spelling).  A
contract on a function taking a parameter pack of reference type was rejected
outright with

    error: 'const' qualifiers cannot be applied to 'Args&'

`view_as_const` constifies a use of a parameter inside a predicate by calling
`cp_build_qualified_type`.  A non-pack reference parameter never reaches that
call: by then it is an already-dereferenced `REFERENCE_REF` whose type is the
referent's.  A pack element is different -- the pack has not been expanded, so
it is still a bare `PARM_DECL` whose type is a `TYPE_PACK_EXPANSION` wrapping
the reference.  Testing `TYPE_REF_P` on the type itself says no, but
`cp_build_qualified_type` recurses through the pack expansion into its
pattern, finds the reference there, and hard-errors.

`contract_ref_or_ref_pack_p` recognises both spellings together, and
`view_as_const` leaves such a parameter alone.  That is correct rather than
merely quiet: at instantiation each expanded element is an ordinary reference
parameter and is constified through the usual path, and a reference is
immutable already -- access *through* it is constified where it is
dereferenced.

## Compile gap

None: one new static predicate and one extra conjunct in an existing
condition.

## Contents

- gcc/cp/contracts.cc : @finish_contract_condition, #18
- gcc/testsuite/g++.dg/contracts/cpp26/pr126878.C : *
