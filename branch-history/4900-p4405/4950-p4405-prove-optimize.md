---
id: 4950-p4405-prove-optimize
subject: 'c++, contracts: D4405: the proof optimization level'
depends: [4940-p4405-override]
regenerates: []
fixes: []
---

## Rationale

D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
E205): how hard the compiler tries to prove the `prove` and `diagnose`
evaluations, independently of the level the code is generated at.

`-fcontracts-prove-optimize=<0|1|2|3|follow>` (C and C++, joined, no
`-fno-` form; an unknown value is an error naming the accepted ones)
selects it.  By default the proof level is the code-generation level but
never below 2: `-O3` and `-Ofast` give 3, and `-O0`, `-O1`, `-O2`, `-Os`,
`-Oz` and `-Og` give 2, since asking to prove means paying for the proof.
`follow` selects the code-generation level itself, where `-Os`, `-Oz` and
`-Og` count as 2.  `c_common_post_options` resolves it into 0-3 and leaves
it in `flag_contracts_prove_optimize`, where the proof shadows can read it.

Each proving strategy is on from a proof level, as `-O` turns on GCC's
optimizations, and a strategy given explicitly, in either form, is kept:

| Strategy | On at proof level |
|---|---|
| `-fcontracts-prove-fold` | 0 and up |
| `-fcontracts-prove-shadow` | 1 and up |
| `-fcontracts-prove-assume` | 1 and up |
| `-fcontracts-prove-override` | 2 and up |
| `-fcontracts-prove-callee-premises` | 2 and up |

So level 0 is folding only, and below level 2 an override pair takes the
`-fno-contracts-prove-override` fallback.  The callee premise copies work
through the proof shadow, so their level is not below the shadow's.
`-fcontract-analysis-skip-dead-calls` is not a proving strategy and is not
in the table.  The defaults apply only under `-fcontracts-p4405`, and at
the default level, 2 or 3, every strategy is on, as before.  Like the
strategies, the option warns that it has no effect without
`-fcontracts-p4405`, whatever its value.

The level also says how hard the proofs try: the proof shadows, and the
copies of the functions they call (`4920-p4405-shadow`), are optimized at
the proof level, 0 included -- a strategy given explicitly at level 0
runs at level 0, optimized as at `-O0` (DECISIONS.md E208 (a)) -- through
the hook the
middle end asks for their options (`contract_shadow_proof_optimization`),
whatever the functions' own levels and `optimize` attributes (E168 (7)).
The options are the command line's with only the level replaced, so what
the user gave applies to the proofs too (E165), and the IPA cloning,
splitting and identical code folding of analysis-only code stay off
(E207).  So the results are the same whatever the level the code is
generated at, and with `-O1`, `-Og`, `-Os` and `-Oz` the proofs are made
at level 2, through level copies of the callees, where the callees' own
level would lose them.  The program's code is the same whatever the proof
level (without `-fcontracts-prove-assume`).  That holds for a `-O0`
compilation too, which proves at level 2 by default (E166 stage 2): its
shadows are optimized at the proof level, they inline the bodies of
`extern template` members, which `-O0` otherwise never analyzes, and
`-O0` drops the assumptions `-fcontracts-prove-assume` makes (E167); at
level 0 (`follow` at `-O0`) it makes none.

Tests: `p4405-prove-optimize-*.C` -- each value, the bad values and the
missing `-fno-` form, the last option winning, the default and `follow` at
each `-O` level, each row of the table, explicit flags in both directions,
and the premise copies, which do nothing without the shadow; a strategy
given explicitly at level 0, whose shadows run at level 0 (the paper's
override shape and the bindels example, at `-O2` and `-O0`); the object
code the same at a fixed proof level from `-O1` to `-Oz`, with and without
the shadows (the shapes of `p4405-shadow-same-code-*.C`); the bindels
example and the premises tests at `-O1`, `-Og`, `-Os` and `-Oz`, and with
no level copy at `-O2` and `-O3`; outlined checkers optimized at `-O0` by
`-fcontract-disable-optimized-checks`; facts that need a level, a
function's own `optimize` and `cold` attributes, a loop pragma and the
unrolling flags (`p4405-prove-optimize-level-restores.C`).  A function's
own `optimize` attribute no longer decides
(`p4405-shadow-O0-attribute.C`, `p4405-shadow-O2-attribute.C`).  At
`-O0`: the default proves (`p4405-shadow-O0-default.C`, the C
`c-p4405-shadow-O0-default.c`, `p4405-prove-optimize-default-O0.C`), the
bindels example, an `extern template` member, IPA pure-const for the
analysis-only code only, alias analysis, a `bool`'s range, inlining and
the user's `-fno-strict-aliasing` and inlining flags as at `-O2`, a
callee's own `noinline` kept and its `optimize` and `cold` attributes not
(`p4405-prove-optimize-attributes-O0.C`), a premise copy of a function
whose postcondition has captures, the same object code at level 0, without the
shadows, with ignore and with `-fcontracts-prove-assume`, for the shapes of
`p4405-shadow-same-code-*.C` and for the statements that hold nothing but
an evaluation (`p4405-prove-optimize-same-code-O0.C`), and the same
diagnostics at `-O0` as at `-O2` for the paper's examples and the nested,
override, premise and unreachable tests (`contracts-same-diagnostics`).
With `-g` the object code, debug information included, is the same with
the shadows of every kind and the copies as without them, at every level
(`p4405-shadow-debug-info*.C`), but for what the regions leave in the
early debug information and the first CFG cleanup (GCC-998, two watch
tests).  The tests whose point is what folding alone decides at `-O0` pass
`-fcontracts-prove-optimize=0`.

## Compile gap

None.  The table names the strategies of `4910-p4405-fold`,
`4920-p4405-shadow` (the shadow and the callee premise copies) and
`4940-p4405-override`; the hook giving the proof shadows their options,
and the function building them, are `4920-p4405-shadow`'s.  The warnings
for the options that have no effect,
which list the option, and its line in the option synopsis arrive with
`6900-cross-paper-options`; the C tests, `c-p4405-prove-optimize-*.c`,
with the C front end's emission points they need, in `5000-p4299`.

## Contents

- gcc/c-family/c.opt : /^; D4405: the proof optimization level\./, /^Name\(contracts_prove_optimize\) /, /^Enum\(contracts_prove_optimize\) String/, /^fcontracts-prove-optimize=$/
- gcc/c-family/c.opt.urls : /^fcontracts-prove-optimize=$/
- gcc/c-family/c-opts.cc : /^#include "contracts-shadow\.h"$/, /^  \/\* The P4405 proof optimization level \(DECISIONS\.md E162-E168, E205\):$/
- gcc/doc/invoke.texi : /^@opindex fcontracts-prove-optimize$/, /^Select the @dfn\{proof optimization level\}, which decides which of the$/, /^So at level 0 only folding runs, and below level 2 a virtual function's$/, /^the optimization level, but at least 2: @option\{-O3\} and @option\{-Ofast\}$/, /^optimization level itself, where @option\{-Os\}, @option\{-Oz\} and$/, /^level, with the options given explicitly, whatever their functions' own$/, /^optimization levels and @code\{optimize\} attributes; a function a shadow$/, /^copy at the proof level, never emitted\.  Without$/, /^@option\{-fcontracts-prove-assume\} the code of the program is the same$/, /^whatever the proof level\.  At @option\{-O0\} the proof shadows also inline$/, /^the bodies of external functions \(such as the inline members of a class$/, /^template whose instantiation is declared @code\{extern template\}\), which$/, /^an unoptimized compilation otherwise ignores and which are never emitted,$/, /^and the code is the same with @option\{-fcontracts-prove-assume\} too:$/, /^@option\{-O0\} drops its assumptions, as it drops every assumption whose$/, /^condition is not simple\.  At proof level 0, which @samp\{follow\} selects$/, /^at @option\{-O0\}, the proof shadows are off unless given explicitly; a$/, /^strategy given explicitly at level 0 runs at level 0, where the proof$/, /^shadows are optimized as at @option\{-O0\} and prove little\.  The option is$/, /^accepted for C and Objective-C too\.$/
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-O0-attribute.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-O2-attribute.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-O0-default.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-debug-info.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-debug-info-premises.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-debug-info-statements.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-debug-info-residue.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-debug-info-scope.C : *
