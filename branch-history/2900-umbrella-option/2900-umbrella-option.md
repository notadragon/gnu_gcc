---
id: 2900-umbrella-option
subject: 'c++: contracts: add the -fcontracts-p3850 umbrella option'
depends: []
regenerates: []
fixes: []
---

## Rationale

Add `-fcontracts-p3850`, a single flag that turns on every paper this branch
implements at once.  It exists only because this is a multi-paper prototype
fork: no such flag would ever go upstream, since upstream ships one paper's
worth of contracts, not ten.  Per-paper flags (`-fcontracts-pNNNN`) stay with
their own papers; this commit adds nothing but the umbrella over them.

**It sits immediately before the first paper, and the position is forced
from both sides.**  It cannot come earlier in any meaningful sense -- it
umbrellas extensions to the base facility, so putting it ahead of that
facility inverts the dependency, and anything below band 1000 is
inside the "GCC has fewer bugs than master" milestone, which an extension
flag plainly is not part of.  It cannot come later either: the nine
`LangEnabledBy(C++ ObjC++,fcontracts-p3850)` clauses below live on their
own papers' `c.opt` lines, and a `.opt` line cannot be split, so deferring
the umbrella to sit beside `6900-cross-paper-options` would make nine
commits each stub it.  2900 is the only slot that costs nothing.

Mechanically it is three things.  In `c.opt`, the new `fcontracts-p3850`
option (`Var(flag_contracts_p3850)`) and a `LangEnabledBy(...,
fcontracts-p3850)` clause added to each of the nine papers that already have
a flag: P3097, P3098, P3099, P3100, P3290, P3400, P4283, P4298 and P4301.
Two flags are deliberately left out -- `-fcontracts-p4299` (P4299 "C++
Contracts for C" is C-only) and P3595, which has no per-paper flag of its own
to enable.  In `g++spec.cc`, `OPT_fcontracts_p3850` joins the switch that
tells the driver to link the experimental runtime, alongside the other flags
that already trigger it, so that `-fcontracts-p3850` alone is enough to
build and link. `invoke.texi` documents the umbrella and cross-references
each per-paper option it implies.  The test,
`p3850-implies.C`, compiles once with only `-fcontracts-p3850` and checks
every implied feature-test macro (and, for P3097, which predefines none, an
actual virtual-function contract) -- it is the thing a reviewer should run
first.

## Compile gap

No identifiers are forward-referenced; this is options, driver plumbing and
documentation text, all self-contained.  The one gap is behavioural, not
structural.

**Behaviour gap: `-fcontracts-p3850` does not yet imply `-fcontracts`.**  The
`LangEnabledBy` expression that turns the base facility on is a single
boolean naming every per-paper flag at once, so it cannot be divided and
lands in `6900-cross-paper-options`.  Until that commit, `-fcontracts-p3850` sets its
own `flag_contracts_*` variable and nothing else, and this commit's own
tests -- which compile with it and expect contracts to be enabled -- fail.
The code is right; only the switch that arms it is missing.  The other half of the umbrella, `-fcontracts-p3850` implying each
per-paper flag, is a `LangEnabledBy` on each paper's own option and
arrives with that paper.

## Contents

- gcc/c-family/c.opt : /^fcontracts-p3850$/
- gcc/cp/g++spec.cc : /^\tcase OPT_fcontracts_p3850:$/
- gcc/doc/invoke.texi : /^Enable all of the contracts extension papers listed below/
- gcc/testsuite/g++.dg/contracts/cpp26/p3850-implies.C : *
