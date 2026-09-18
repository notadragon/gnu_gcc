---
id: 4020-instantiate-on-use
subject: "c++: contracts: substitute a template's contracts on odr-use, not only on instantiation"
depends: [2400-p3595, 3000-p3097, 4000-p3098]
regenerates: []
fixes: []
---

## Rationale

Fixes GCC-34 and GCC-35, two faces of the same defect: needing a function's
CONTRACTS is not the same as needing its DEFINITION, and GCC's deferral
model conflated them.  GCC defers contract substitution to
`instantiate_body` -- `tsubst_function_decl` deliberately copies a pattern's
contract specifiers onto an instantiation *unsubstituted*, and
`regenerate_decl_from_template` does the real substitution once the
definition is instantiated.  That is fine as long as something eventually
instantiates the definition; two readers this branch introduces do not:

* P3097 evaluates a virtual function's interface contracts in a wrapper
  around the vtable dispatch, and a *pure* virtual has no definition to
  instantiate at all (GCC-34).
* P3595's caller-side checking emits the check at the call site, so a
  function template that is declared and called but never defined has its
  contracts read with no definition in sight either -- and the call can
  still link if the call is optimized away, so nothing else would ever
  diagnose it (GCC-35).

`maybe_instantiate_contracts` (`pt.cc`) is the single fix for both: given a
`FUNCTION_DECL`, it finds the pattern via the pre-existing
`template_for_substitution`, and substitutes the pattern's contract
specifiers onto the instantiation if that has not already happened.  It
early-returns once the pattern has a body -- `regenerate_decl_from_template`
will substitute those contracts when the deferred definition is eventually
instantiated, and doing it twice here as well would double every
diagnostic a predicate can raise, including the constification errors and
the [dcl.contract.func]/7 const-parameter check.  What is left is exactly
the never-defined case both bugs share.

The one call site is `mark_used` (`decl2.cc`), placed after `DECL_ODR_USED`
is set: [dcl.contract.func]/9 needs a function's contracts substituted when
it is odr-used, and `mark_used` is where GCC already recognizes an odr-use.
Setting the flag before substituting is what makes this safe against a
predicate that odr-uses its own function, directly or through another
contracted one -- a re-entrant `mark_used` call hits the `DECL_ODR_USED`
early return instead of recursing into `maybe_instantiate_contracts` again.
Clang made the mirror-image mistake, gating its own odr-use hook on
`OdrUse == Used` and missing the pure-virtual case entirely; the companion
Clang fork carries the corresponding fix.

This is a **backwards dependency from an earlier paper**: `3000-p3097`
precedes this commit in the linear history and already calls
`maybe_instantiate_contracts` from `define_one_contract_wrapper_func` (via
`DECL_ORIGIN`), because a wrapper built around a class-template member's
contract needs exactly this substitution.  `3000-p3097`'s own Compile gap
describes carrying a declaration and a no-op stub for that reason; this
commit is what it forward-referenced, and it is the real implementation
that stub is replaced by.

`contract-instantiate-on-use.C` mirrors
`clang/test/Contracts/contract-instantiate-on-use.cpp` and pins the
regression directly: a pure virtual with a non-constant predicate (a call,
or a type-dependent expression), and the same shape with a P3098 capturing
postcondition.  `pre (true)` does not exercise either defect -- a predicate
that is already a constant needs no substitution to be usable at all, so it
passes with or without this fix, and every predicate in the test therefore
contains something substitution has to rewrite.

## Compile gap

None.  `maybe_instantiate_contracts` only calls functions that already exist
in the base P2900 facility at the branch's root -- `template_for_substitution`,
`get_fn_contract_specifiers`, `contract_parameter_pattern`,
`tsubst_contract_specifiers`, `push_access_scope` / `pop_access_scope` -- and
its one caller, `mark_used`, is likewise pre-existing.  The `extern`
declaration this commit adds to `contracts.h` is exactly the one
`3000-p3097`, earlier in this history, already carries its own copy of as a
stub; from here on there is one definition, not two.  This is a targeted
check of the functions this entry's Contents names, not an exhaustive audit
of every caller in `decl2.cc` or `pt.cc`.

## Contents

- gcc/cp/contracts.h : @maybe_instantiate_contracts
- gcc/cp/decl2.cc : @mark_used
- gcc/cp/pt.cc : @maybe_instantiate_contracts, @template_for_substitution
- gcc/testsuite/g++.dg/contracts/cpp26/contract-instantiate-on-use.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-instantiate-on-use.json : *
