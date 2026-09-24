---
id: 1020-constexpr-side-effect
subject: 'c++: contracts: P2900: a side-effecting predicate is still a constant expression'
depends: []
regenerates: []
fixes: [gcc-42]
---

## Rationale

A contract predicate evaluated during constant evaluation runs under a
`modifiable_tracker`, which refuses any store to an object that belongs to the
enclosing evaluation.  Upstream treated that refusal as "not a constant
expression" and rejected the program.  That is wrong twice over:
[basic.contract.eval] *permits* an evaluation that produces the predicate's
value without its side effects but does not require one, and a predicate that
does modify the enclosing evaluation is still a core constant expression, so
rejecting it rejects a well-formed program.

The fix separates the two outcomes the tracker can have.  `constexpr_global_ctx`
gains `modifiable_rejected` and `modifiable_rejected_obj`, set the first time a
store is refused, so a caller can tell "this subexpression modifies something
outside itself" apart from "this subexpression is not constant";
`modifiable_tracker` saves and restores them and exposes them as `rejected ()`
and `rejected_obj ()`.  The contract case in `cxx_eval_constant_expression`
then tries the side-effect-free evaluation first and, if and only if that
failed *because* of a refused store, re-runs the predicate for real and lets
the modification stand.

Because the meaning of such a program then depends on the evaluation semantic
-- under `ignore` the predicate is not evaluated at all -- the retry emits
`-Wcontract-constexpr-side-effect`, on by default.  Two details are
deliberate and worth a reviewer's attention: the warning is not gated on
`ctx->quiet` or on manifestly-constant evaluation, because a *successful*
constant evaluation is performed quietly and with `mce_unknown`, so either
test would silence the warning on exactly the code worth warning about; and it
deduplicates per location, since a contract in a compile-time loop or in a
template reaches this repeatedly.

The option itself is declared here.  Its `c.opt` block is shared with two
warnings from other papers, so `branch-history/splits.md` cuts that hunk and
this commit claims only its own three lines.

`modifiable_tracker`'s own nest-safety is *not* part of this commit and is no
longer part of this branch: upstream saves and restores `previous_set` itself.
That matters to the reading of the code here, because the `previous_rejected*`
state added below only composes correctly because the set beside it is
restored the same way.

## Compile gap

None.  The `c.opt` cut means `-Wcontract-constexpr-side-effect` and its
`warn_contract_constexpr_side_effect` variable are declared by this commit
rather than borrowed from P3400, so no placeholder option has to be carried
and later removed.

The two nesting tests added here exercise upstream's nested-tracker fix as
well as this one -- a tracker's destructor restoring the enclosing set is what
makes the `[[assume]]` rows come out zero.  They live here because they need
`-fcontracts` and match this commit's `-Wcontract-constexpr-side-effect`,
neither of which exists upstream, so they could not go with that patch.

## Contents

- gcc/c-family/c.opt : /^Wcontract-constexpr-side-effect$/
- gcc/cp/constexpr.cc : /Set when MODIFIABLE actually refused a modification/, /modifiable_rejected \(false\), modifiable_rejected_obj \(NULL_TREE\),/, /modifiable_rejected_obj = DECL_P/, /^  bool previous_rejected;$/, /previous_rejected = global->modifiable_rejected;/, /^    global->modifiable_rejected = previous_rejected;$/, /Whether a modification was refused while this tracker was active/, /\[basic.contract.eval\] permits, but does not require/, /^\tbool modifies_outside = false;$/, /No such side-effect-free evaluation exists/, /The modification stood, so the meaning of the program now/
- gcc/doc/invoke.texi : /^Warn when the predicate of a contract assertion/, /modifies an object of the enclosing evaluation/, /the modification is performed/, /^This warning is limited to constant evaluation/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect-nested.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect-warn.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-nested-modifiable-tracker.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-assume-nesting-permutations.C : *
