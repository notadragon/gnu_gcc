---
id: 1010-constexpr-named-result
subject: 'c++: contracts: P2900: bind a postcondition result name during constant evaluation'
depends: []
regenerates: []
fixes: [gcc-56]
---

## Rationale

A postcondition with a result-name-introducer, `post (r : p (r))`, does not
name a function parameter: `r` is a synthetic variable the front end builds
for the predicate.  The constant evaluator binds parameters by walking the
call's argument list, so nothing ever bound `r`, and evaluating such a
postcondition in a constant expression read an unset value.

The fix is three pieces of one mechanism.  `constexpr_call` gains a
`result_decl` field holding the remapped `RESULT_DECL` of the body copy this
call is evaluating; `cxx_eval_call_expression` sets it where it already has
that decl to hand; and `cxx_eval_constant_expression`, on reaching a
`POSTCONDITION_STMT`, binds the postcondition's own result variable to that
`RESULT_DECL` before evaluating the predicate.

What the binding holds depends on the type, following the rule
`cxx_bind_parameters_in_call` already uses for a real parameter: for a type
that must live in memory the map holds the object, and for any other type it
holds the value.  The `PARM_DECL` case of `cxx_eval_constant_expression`
depends on that distinction -- a glvalue use of an addressable-typed parm
returns the mapped entry unchanged, a prvalue use evaluates it -- so binding
the value for an addressable type would leave the result name denoting a
`CONSTRUCTOR` with no storage.  Reading a member through such a binding works,
but every member function call passes `this`, so any call in the predicate
would reach the `ADDR_EXPR` case with a `CONSTRUCTOR` operand and trip its
`gcc_checking_assert`.

A reviewer should check that the binding happens on the postcondition
statement and not earlier -- the return value does not exist until the body
has run -- and that `POSTCONDITION_IDENTIFIER` is tested for being a `DECL`
before it is used as a key, since a postcondition without an introducer
carries no identifier at all.

## Compile gap

None.  Every field this commit adds it also initialises and reads; nothing
here is scaffolding for a later commit to replace.

## Contents

- gcc/cp/constexpr.cc : /The \(remapped\) RESULT_DECL of this call's body copy/, /Make the call's result decl reachable/, /A named-result postcondition refers to a synthetic result variable/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-postcondition-constexpr.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-named-result-address.C : *
