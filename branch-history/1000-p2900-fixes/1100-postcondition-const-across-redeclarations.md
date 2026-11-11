---
id: 1100-postcondition-const-across-redeclarations
subject: 'c++: contracts: P2900: carry the postcondition const rule across every declaration'
depends: [1060-postcondition-odr-use]
regenerates: []
fixes: [gcc-01, gcc-26]
---

## Rationale

Two bugs in one commit, because they are two halves of one edit.
`check_postconditions_in_redecl` walks the parameter lists of an old and a new
declaration in lockstep, propagating "odr-used in a postcondition" and
checking [dcl.contract.func]/7 on the new one.  Both bugs are in that walk,
and the fix for each is inside the function the other one extracts.

GCC-1 (PR c++/124395) is the walk itself.  A function parameter pack occupies
one slot in the pattern but expands to N parameters in the instantiation, so
the two lists cannot be walked in lockstep throughout: a pack that expands to
nothing leaves the new list shorter, and a parameter written after a pack sits
at a different index in each.  The old walk marched off the end and either
ICEd or attributed a diagnostic to the wrong parameter.  What does correspond
however the packs expand is the run *before* the first pack, aligned from the
front, and the run *after* the last pack, aligned from the back; the walk now
does those two runs and skips the packs, whose expanded elements are checked
by the walk over the substituted predicate instead.  Only a parameter written
between two packs is left unchecked, which takes a second function parameter
pack -- one that can never be deduced, and so never expands.

GCC-26 (PR c++/127196) is which declaration the rule is recorded against.
Either declaration may be the one `duplicate_decls` discards, and which it is
depends on things this function cannot see (a definition's parameters win), so
recording only the old one loses the middle declaration of three.  Both are
now recorded -- recording is idempotent, and a surviving parameter is checked
directly anyway -- and `check_postcondition_redecl_parm_types` re-checks the
recorded, merged-away parameters once a template's arguments are known, which
is the only point at which a dependent parameter type can be judged const or
not.

The indivisible part: extracting the loop body into
`check_postcondition_parm_in_redecl` is GCC-1's refactor, and the two
`record_postcondition_redecl_parm` calls that make GCC-26 work sit inside that
extracted body, between its two GCC-1 halves, with no unchanged line between
them.  Cutting there would leave one piece that is a function doing nothing
and another that is a fragment of a function.  They ship together.

A parameter whose type became a reference by substitution is not a value
parameter, and is exempt (`g<int &>`, `postcondition-redecl-reference-param.C`).

The parameters of the declarations merged away are recorded whether or not
the pattern's predicate has marked the parameter yet, and
`check_postcondition_redecl_parm_types` runs again from `tsubst_contract`
after the walk over the substituted predicate, judging each parameter of the
instantiation once: a member access through a dependent object is only known
to odr-use its object then (GCC-663).

## Compile gap

None.  Both new helpers, the forward declaration they need, the `contracts.h`
export and the `tsubst_function_decl` call site are all here.

## Contents

- gcc/cp/contracts.cc : @record_postcondition_redecl_parm, @check_postcondition_parm_in_redecl, @check_postcondition_redecl_parm_types, /^  tree first1 = FUNCTION_FIRST_USER_PARM \(olddecl\);$/, /^  \/\* A function parameter pack occupies a single slot in the pattern \(OLDDECL\)$/, /^static GTY\(\(\)\) hash_set<tree> \*postcondition_redecl_parms_checked;$/
- gcc/cp/contracts.h : @check_postconditions_in_redecl
- gcc/cp/pt.cc : @tsubst_function_decl, /That walk can be the first to find a parameter odr-used/
- gcc/testsuite/g++.dg/contracts/cpp26/dcl.contract.res.p1-pack-empty.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr127196.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-redecl-reference-param.C : *
