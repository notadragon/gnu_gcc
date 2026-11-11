---
id: 6970-refinable-body-fork-tests
subject: 'c++, contracts: test refinable bodies across the extensions'
depends:
  - 0430-refinable-body-contracts
  - 2400-p3595
  - 3000-p3097
  - 4230-p3400-facet-allowed-semantics
  - 4800-p4298
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

Multi-unit run tests for the refinable-body mark on this branch's own
paths, which need several papers at once and so come after all of them.
Each links a unit holding callers, compiled with one definition of an
inline function, with a unit holding another definition that the linker
keeps; a configuration file gives each assertion its semantic by
location.

`refinable-body-enforce-replaceable.C`: an enforced check's noreturn
violation entry point gives an inline function a return value range in
its unit; a caller there that calls it out of line must not fold its
comparison on that range when the kept definition ignores the check.

`refinable-fork-sem.C`: every evaluation semantic.  Callers compiled with
an enforce, noexcept_enforce, quick_enforce (an inline trap) or assume
evaluation must not fold the function's return value range when the kept
definition ignores or observes the assertion; callers compiled with an
ignore, quick_enforce, noexcept_enforce, noexcept_observe or assume
evaluation, or a dynamic selection (P3595) that selects ignore, must keep
their catch when the kept definition's handler throws.

`refinable-fork-label.C`: P3400 labels.  {enforce, observe} and {enforce,
quick_enforce} on a function that may throw do not settle the evaluation,
so the same two failures must not happen; the settled sets --
{quick_enforce, noexcept_enforce, assume}, {enforce, quick_enforce} on a
noexcept function, {enforce} alone -- run with valid arguments.

`refinable-fork-wrapper.C`: a caller-side check (P3595 configuration) and a
P3097 wrapper, both out of line (`-fno-inline`), give their caller a
return value range; the kept definition of the caller checks nothing.

`refinable-body-p3097.C`: a caller of a P3097 wrapper -- for an ordinary
virtual function, a C-variadic one and an explicitly called virtual
destructor alike -- is marked unless every evaluation of the wrapper is
settled, so its caller keeps or drops its landing pad accordingly; a
virtual call of a function without contracts marks nothing.

`refinable-body-p3097-unit-config.C`: a P3097 wrapper's checks are
configured per unit, and a unit that ignores them makes no evaluation, yet
its callers stay correct: a non-noexcept virtual call may throw, so they
keep their handler, and a noexcept one's kept wrapper terminates anyway.

`refinable-body-p3100.C`: P3100 implicit checks do not mark their function
(going past one is undefined in every unit), so its caller drops its
landing pad.

## Compile gap

None.  Pure test additions.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-enforce-replaceable* : *
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-fork-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-p3100.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-p3097.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-p3097-unit-config* : *
