---
id: 1200-xobj-member-in-predicate
subject: 'c++: contracts: P2900: do not claim a constructor/destructor rule in an xobj function'
depends: []
regenerates: []
fixes: [gcc-28]
---

## Rationale

GCC-28 (PR c++/127294).  Naming a member unqualified in the contract of an
explicit object member function was diagnosed with the constructor/destructor
message -- "a member of a class may not be named in a constructor precondition
or destructor postcondition unless through an explicit `this`" -- which is
about a rule that has nothing to do with the function in question.

`contract_class_ptr` is set only for a constructor precondition or a
destructor postcondition.  The test compared it against `current_class_ptr`,
and in an explicit object member function, which has no `this` at all, both
are null, so the comparison succeeded and the wrong diagnostic fired.  Both
sites -- the `FIELD_DECL` case and the member-function-set case -- now test
`contract_class_ptr` for non-nullness first.  Falling through instead gives
such a function the same "invalid use of non-static data member" diagnostic
its body already gets, which is the point: the contract and the body should
agree.

## Compile gap

None: two added conjuncts.

## Contents

- gcc/cp/semantics.cc : #3, #4, #5, #6
- gcc/testsuite/g++.dg/contracts/cpp26/deducing-this-no-this-in-predicate.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/deducing-this-postcondition-param.C : *
