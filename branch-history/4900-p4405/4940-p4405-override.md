---
id: 4940-p4405-override
subject: 'c++, contracts: D4405: override shadows of virtual functions'
depends: [4930-p4405-analysis]
regenerates: []
fixes: []
---

## Rationale

D4405R0, "Virtual Functions".  A call through a base class `B` evaluates
the contract assertions of `B::f`, which the caller sees, and those of the
final overrider `D::f`, which it does not, so no caller can analyze whether
`D::f` honors the contract it overrides.  For each function `B::f` that a
virtual function `D::f` defined in the translation unit overrides, directly
or indirectly, an override shadow of `D::f`'s body takes `B::f`'s
preconditions as given and attempts to prove, in order, `D::f`'s
preconditions at entry, `D::f`'s postconditions at each return, each ahead
of its own check, and `B::f`'s postconditions after all of `D::f`'s checks.
Each obligation is an analysis evaluation of the assertion being proven,
with its own analysis semantic, resolved against the function it belongs
to; when all are `ignore` there is no override shadow.  A `B::f` without
contract assertions gets one too, with nothing given, so `D::f`'s
preconditions must hold for any argument.

`begin_override_shadows`, called from `maybe_apply_function_contracts`,
finds the overridden functions by walking every base class of `D` and
asking each with `look_for_overrides_here` (every overridden function, not
only the first on each path), registers one override pair per overridden
function with an obligation -- per base class subobject when `B::f`'s
contract assertions name `this`, which then reads a different object in
each, a virtual base counting once
(`contract_shadow_register_override`), and offers each obligation with the
pair (`diagnose_unproven_contract`), placing the strategy's marker where
the evaluation happens.  At entry, per pair that placed a marker: the
snapshots the pair's regions read, `B::f`'s preconditions as assumptions,
with `B::f`'s parameters replaced by `D::f`'s by position and `this` by
`D::f`'s converted to the base class subobject (`build_base_path`, through
the vbase offset for a virtual base), skipped when the predicate holds
anything that cannot be copied this way, then `D::f`'s preconditions.  An
assumption is guaranteed only when the precondition's label settles it
(`contract_premise_settled_p`): every caller evaluates it, in any
translation unit.  `apply_postconditions` places each of `D::f`'s
postconditions' obligations ahead of its check
(`add_override_post_obligations`), and the try/finally of the checks has
`B::f`'s after them (`add_override_base_post_obligations`), which also puts
the regions at entry in place.  In `B::f`'s postconditions `this` is a
snapshot of the converted `this`, a parameter `D::f` receives by value but
not by invisible reference is a snapshot taken at entry -- the P3097
wrapper checks the postcondition with a copy of its own -- and the result
is `D::f`'s, converted for a covariant return as the covariant thunk does,
null-checked for a pointer.  The snapshots are assigned only in the pair's
regions.  A `D::f` without active postconditions gets the regions at entry
alone, or, when `B::f`'s postconditions placed a marker, a try/finally of
their own (`place_override_shadows_alone`); the real function sheds the
regions before SSA, and its code is unchanged (compared, with and without
the override strategy and assumptions, against analysis `ignore` by
`contracts-same-object-code` in `p4405-override-same-code.C`,
`-realcode.C` and `-dtor-realcode.C`).  With
`-fcontract-checks-outlined` the regions stay in `D::f`.  A postcondition
with captures is decided at once.

The override shadows are a proving strategy of their own,
`-fcontracts-prove-override` (`contracts-prove-override.cc`, implied by
`-fcontracts-p4405`), since each costs an optimization of the body; it
defers the evaluations made in an override shadow, using the proof
shadows' markers and premise regions.  Without it, the pairs' evaluations
are obligations of `D::f` itself, with nothing taken from `B::f`'s
preconditions: `D::f`'s preconditions are decided at once, by folding;
`D::f`'s postconditions need no more than their own analysis evaluations;
and `B::f`'s postconditions, rewritten as above, are proven in `D::f`'s
ordinary shadow.  The middle end (`4920-p4405-shadow`) makes one shadow per
pair, decides the obligations, never assumes an override proof in the real
code, and diagnoses an unproven one at `D::f`'s definition, naming `B::f`,
only if `D::f` is compiled.  A pure virtual or undefined `D::f` has no body
to analyze.

A virtual destructor overrides too, an implicitly defined one included.
Its body for the override shadows is what it runs, its own statements and
then the destruction of its members and bases, so `B::~B`'s postconditions
are proven after `B::~B` has run; the deleting destructor only calls the
complete one.  In a class with virtual bases the regions are placed under
the in-charge test (`override_in_charge`), so only the complete-object
destructor holds them: it is what a `delete` through a base class runs,
while the base-object destructor, which does not destroy the virtual
bases, is never the target of a virtual call.

The tests: the paper's example, with and without the strategy; a hierarchy
of several levels, several bases, a virtual base reached twice, a pure
virtual `B::f`, a `B::f` without contracts and an undefined `D::f`, and the
return side through several levels and bases; `D::f`'s postconditions
proven through `B` only, and never by their own checks; `B::f`'s
postconditions in an overrider without contracts, by label, through a
covariant return, with a modified parameter, for noexcept, throwing and
named-return-value bodies, and with outlined checks; diagnose; sound and
conditional proofs by `B::f`'s label; no assumption in the real code while
an ordinary sound proof next to an override shadow is still assumed; an
overrider not compiled in the translation unit, also at `-O0`, also for
postconditions and destructors; without the proof shadows, without the
override shadows, and without both; a constexpr virtual function;
destructors, user-provided and implicit, with virtual bases, with and
without the override shadows, constexpr, and their real code; repeated
base class subobjects; templates; `final`; and postconditions with
captures.

## Compile gap

None.  The front end uses `4900-p4405`'s proving interface (the query's
override pair and override shadow, the pair argument of the assumption),
`4920-p4405-shadow`'s override pairs, shadows and regions.

## Contents

- gcc/cp/contracts.cc : /^#include "contracts-shadow\.h"$/, /^  \/\* D4405: the override shadows \(begin_override_shadows\), by themselves$/, /^    start_override_shadows \(overrides, cur_stmt_list\);$/, /^      add_override_post_obligations \(NULL_TREE\);$/, /^	add_override_post_obligations \(orig_contract\);$/, /^      add_override_base_post_obligations \(\);$/, /^\/\* D4405R0, "Virtual Functions": override shadows\.  A call through a base$/, @find_this_parameter_r, @contracts_name_this_p, @collect_overridden_functions, @override_pair, @override_shadows, @current_override_shadows, @make_override_snapshot, @override_region, @override_in_charge, @register_override_pair, @override_kind, @override_parm_snapshot, @override_this_snapshot, @override_result, @override_remap_data, @override_remap_r, @map_override_parms, @override_obligation_p, @offer_override_obligation, @build_override_pre_obligations, @build_override_post_obligations, @build_override_base_post_obligations, @override_shadows_post_p, @add_override_post_obligations, @override_entry_regions, @prepend_override_shadow_regions, @end_override_shadows, @finish_override_shadows, @start_override_shadows, @add_override_base_post_obligations, @place_override_shadows_alone
- gcc/cp/contracts.h : /^struct override_shadows;$/
- gcc/c-family/contracts-prove-override.cc : *
- gcc/c-family/contracts-prove.cc : /^  &contract_prove_override_strategy,$/
- gcc/c-family/contracts-prove.h : /^\/\* -fcontracts-prove-override \(contracts-prove-override\.cc\): the override$/, /^extern const contract_proof_strategy contract_prove_override_strategy;$/, /^extern bool contract_proof_override_shadows_p \(tree fndecl\);$/
- gcc/c-family/c.opt : /^fcontracts-prove-override$/
- gcc/c-family/c.opt.urls : /^fcontracts-prove-override$/
- gcc/Makefile.in : /^  c-family\/contracts-prove-override\.o \\$/
- gcc/doc/invoke.texi : /^A call through a base class evaluates the contract assertions of the$/, /^overridden function @code\{B::f\}, which the caller sees, and those of the$/, /^final overrider @code\{D::f\}, which it does not \(@option\{\-fcontracts\-p3097\}\)\.$/, /^So the body of each virtual function @code\{D::f\} defined in the translation$/, /^unit is analyzed once more for each function @code\{B::f\} it overrides,$/, /^directly or indirectly, in an @dfn\{override shadow\} that takes the$/, /^preconditions of @code\{B::f\} as given \(@option\{\-fcontracts\-prove\-override\}\),$/, /^and in which, in order, each precondition of @code\{D::f\} is analyzed at$/, /^entry, each postcondition of @code\{D::f\} at each return, ahead of its own$/, /^entry in a postcondition of @code\{B::f\} unless @code\{D::f\} receives it by$/, /^reference or by invisible reference \(a class with a nontrivial copy$/, /^constructor or destructor\), since the check of that postcondition evaluates$/, /^it with its own copy; and the result of @code\{D::f\} is converted to the$/, /^return type of @code\{B::f\} for a covariant return\.  A @code\{B::f\} without$/, /^in the code \(@option\{\-fcontracts\-prove\-assume\}\): @code\{D::f\} also serves$/, /^calls that never established the preconditions of @code\{B::f\}, and the$/, /^translation unit does not compile \-\- an inline function whose class's$/, /^virtual table another translation unit emits \-\- is analyzed there and not$/, /^diagnosed here\.  A pure virtual or undefined @code\{D::f\} gets no override$/, /^shadow\.  A virtual destructor, an implicitly defined one included,$/, /^overrides the virtual destructors of its bases; their postconditions are$/, /^analyzed after it has destroyed its members and bases, in the$/, /^complete\-object destructor only \(the one a @code\{delete\} through a base$/, /^class runs\)\.  A base class occurring as more than one subobject gives one$/, /^override shadow per subobject when the contract assertions of @code\{B::f\}$/, /^name @code\{this\}, and a single one otherwise\.  A postcondition with captures$/, /^Without override shadows \(at @option\{\-O0\}, or with$/, /^of @code\{D::f\} itself, with nothing taken from the preconditions of$/, /^analysis evaluation, and the postconditions of @code\{B::f\} are analyzed in$/, /^@opindex fcontracts\-prove\-override$/, /^A proving strategy of its own, enabled by @option\{\-fcontracts\-p4405\} and$/, /^disabled by @option\{\-fno\-contracts\-prove\-override\}: analyze the body of$/, /^override shadow costs one more optimization of the function\.  Its shadows$/
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-* : *
