---
id: 6010-p3100-assume-codegen
subject: 'c++, contracts: P3100: lower the assume semantic as an optimizer assumption'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

An evaluation of a contract assertion with the P3100 `assume` semantic
(allowed by `-fcontracts-allow-assume`) is an optimizer assumption of its
predicate, lowered as `[[assume]]` lowers its own: `IFN_ASSUME`, which the
gimplifier turns into `if (!pred) __builtin_unreachable ()` for a simple
predicate and otherwise outlines into an assumption function that is never
called.  So the predicate is never evaluated -- one with side effects
included -- and the optimizers may take it to hold where the evaluation
happens.

`build_contract_assumption` builds it, from `emit_check_for_semantic`'s
`assume` arm (which shares `ignore`'s `return void_node` for everything
else), with `this` and a postcondition's result remapped as for a check.
`contract_lowered_as_assumption_p` says which evaluations it applies to:
every `assume` evaluation but a postcondition with captures, whose
captures are initialized only for an evaluation that checks the predicate.
`contract_active_p` keeps those contracts in the body, so preconditions and
postconditions reach the emitter; at the call, a caller-side `assume`
precondition makes the wrapper be built, and inlining the wrapper brings
the assumption into the caller (`maybe_contract_wrap_call`, and
`define_one_contract_wrapper_func` for a postcondition).  Constant
evaluation is unchanged: `assume` is skipped there, as before.  A
dynamically selected `assume` still takes the `ignore` arm (the selection
happens at run time, after the optimizers).

The tests show a test each kind of assertion rules out removed as dead
code, including a predicate that calls a function, the predicate never
evaluated (inline and outlined checks, caller side), and the P4405 proof
shadow assuming an `assume` precondition or postcondition as a guaranteed
premise.

## Compile gap

None.  The C front end's `assume` lowering arrives with `5000-p4299`, with
its test.

## Contents

- gcc/cp/contracts.cc : @all_contracts_statically_nonthrowing:1, @contract_lowered_as_assumption_p:0, @contract_active_p:1, @maybe_contract_wrap_call:6, #140:1, @contract_local_handler_always_rethrows_p:1, @build_contract_assumption:0, #223:1
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-semantic-assume.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-semantic-assume-caller.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-semantic-assume-caller.json : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-semantic-assume-outlined.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-semantic-assume-run.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-assume.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-assume.json : *
