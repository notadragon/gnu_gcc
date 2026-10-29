---
id: 0220-ctor-pre-qualified-member
subject: 'c++: contracts: reject a qualified member name in a constructor precondition'
depends: []
regenerates: []
fixes: [gcc-574]
---

## Rationale

GCC-574, qualified half.  In a constructor precondition or a destructor
postcondition a potentially evaluated id-expression naming a non-static
member is transformed to a class member access through `this` and is
ill-formed ([expr.prim.id.general]).  `finish_id_expression_1` diagnosed
only the unqualified name; `pre(C::b)` went straight to
`finish_qualified_id_expr` and was accepted.

The qualified-id branch now applies the same test for a data member or a
non-static member function of the class or a base, unless the operand is
unevaluated or the name forms a pointer to member (`&C::b`).

## Compile gap

None.  `contract_class_ptr`, `processing_contract_condition` and
`shared_member_p` are upstream's.

## Contents

- gcc/cp/semantics.cc : #13
- gcc/testsuite/g++.dg/contracts/cpp26/ctor-pre-qualified-member.C : *
