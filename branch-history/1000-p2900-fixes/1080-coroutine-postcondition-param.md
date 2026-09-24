---
id: 1080-coroutine-postcondition-param
subject: "c++: contracts: P2900: reject a parameter odr-used in a coroutine's postcondition"
depends: [1060-postcondition-odr-use]
regenerates: []
fixes: [gcc-30]
---

## Rationale

GCC-30 (PR c++/127296).  [dcl.fct.def.coroutine]/5 says a coroutine behaves as
if the top-level cv-qualifiers on all its parameters were removed, so a
by-value parameter is never truly const inside the coroutine's real
definition, however it is spelled.  [dcl.contract.func]/7 requires such a
parameter to be const once a postcondition odr-uses it.  The two cannot both
be met, and the standard states the consequence outright in a note
([dcl.fct.def.coroutine]/6): an odr-use of a non-reference parameter in a
coroutine's postcondition assertion is ill-formed.  GCC accepted it and
generated code against the (moved-from, promise-owned) copy.

The check cannot live where the const rule lives, because a function is not
known to be a coroutine until its body has been parsed, which is after its
contracts.  `diagnose_coroutine_postcondition_params` therefore runs from
`finish_function`, once the coroutine-ness is settled, over the flags
`check_postcondition_param_odr_uses` left on the parameters.

A reviewer should check the exclusions: a reference parameter is fine, since
the coroutine's copy is bound to the same object, and a precondition is not
subject to the const rule at all.

## Compile gap

None.  The pass is new, its declaration is added to `contracts.h` here, and
its single caller in `finish_function` is added here.

## Contents

- gcc/cp/contracts.cc : @check_param_in_postcondition:1, @diagnose_coroutine_postcondition_params
- gcc/cp/contracts.h : @copy_and_remap_contracts
- gcc/cp/decl.cc : @finish_function
- gcc/testsuite/g++.dg/contracts/cpp26/coroutine-postcondition-param.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/coroutine-pre-post.C : *
