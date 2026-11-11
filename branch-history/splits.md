# Segment cuts

Declared interior boundaries for segments that anchors.py cannot divide.

anchors.py cuts a run of added lines at column-0 declaration starts, so it
separates a run of several definitions but is blind INSIDE one definition's
body.  A segment whose interior belongs to more than one planned commit is
declared here, once, and every commit claiming a piece of it sees the same
division -- two commits declaring conflicting cuts of one segment would be
unresolvable, so the declaration deliberately does not live in the commit
entries.

Format: a level-2 heading naming a segment key, then an ordered list of cut
specs.  `+/regex/` matches the first added line of the new piece and is
required; `-/regex/` matches the first removed line and is optional -- when
omitted, every removed line stays with piece 0.  N cut specs make N+1
pieces, numbered from 0, keyed `path#ordinal[.piece]:cut`.

The regexes anchor on content, never on line numbers, so a rebase does not
invalidate them.  A regex that stops matching, matches twice, or falls out of
order is reported by `branch_history.py check` under the `cuts` invariant,
and generation refuses.

## gcc/cp/contracts.cc#80.0
The forward declarations of three `2200-libcontracts` helpers run on into the
heading of the P3098 capture state.  Cut at the heading.

- +/^\/\* The state kept for postcondition captures \(P3098\)\.  \*\/$/

## gcc/cp/contracts.cc#160.12
`resolve_contract_label` is a single 361-line contiguous insertion whose
interior belongs to several different commits.  Cut it after the
local_violation_label trampoline and again before the allowed_semantics
restriction, so the queryable_label facet and the semantic-restriction logic
can be claimed separately.  Cut out, too, the label's location taken at the
top and the reading of the group names at the end, which belong to the
group_names facet.

- +/^  \/\* Where the label is, before it may be materialized below\.  \*\/$/
- +/^  \/\* Use the main variant as the trampoline-map key/
- +/^  \/\* Check for queryable_label facet/
- +/^  \/\* Compute the flag-independent label restriction/
- +/^  \/\* Read the label's group names now, once/

## gcc/c-family/c.opt#0

One hunk declares three unrelated warnings in a row --
`-Wcontract-configuration` (P3595), `-Wcontract-invalid-label-facet`
(P3400) and `-Wcontract-unproven` (P4405).  Nothing but their alphabetical
neighbourhood puts them together.  Cut on the second and third option
names so each commit declares its own.

- +/^Wcontract-invalid-label-facet$/
- +/^Wcontract-unproven$/

## gcc/cp/contracts.cc#31.0

This hunk replaces the body of the old `check_param_in_postcondition` with the
walk that `check_postcondition_param_odr_uses` performs (the postcondition
odr-use rework), and then, in the same run of added lines, opens the
documentation comment of `diagnose_coroutine_postcondition_params`, which
belongs to the coroutine fix.  Cut at the comment so each half travels with
the function it describes.

- +/^\/\* \[dcl\.fct\.def\.coroutine\]\/5 says a coroutine behaves as if/

## gcc/cp/contracts.cc#67

`start_function_contracts`'s `contract_any_active_p` early-out runs straight
into the block that records a function whose postconditions can throw as
having a cleanup that might throw, with no unchanged line between.  The
early-out is the dynamic-selection work's activity query; the recording is
what makes the single return-value cleanup possible.  Cut at the comment so
each half travels with the commit that needs it.  Ahead of both, the
handling of a result name that failed to parse replaces the upstream shadow
check (check_result_name_conflict now does that where the result name is
made); cut it off so it goes with that fix, together with the removed lines.

- +/^  if \(!contract_any_active_p \(fndecl\)\)$/
- +/^  \/\* A postcondition check runs after the returned object has been$/

## gcc/cp/contracts.cc#84

The body of `apply_postconditions`' outlined branch (`1000-p2900-fixes`),
behind D4405's override shadows' obligations of the postconditions, which
stay in the function (`4940-p4405-override`).  Cut at the call, with the
removed upstream lines.

- +/^      add_post_condition_fn_call \(fndecl\);$/ -/^      if \(tree contract_copy = copy_contracts \(fndecl, cmk_post\)\)$/

## gcc/cp/contracts.cc#85.0

A single 105-line insertion that rewrites the body of `apply_postconditions`
and then, with no unchanged line between, opens the documentation comment of
`wrap_postconditions_in_retval_cleanup`.  The rewrite belongs to the
result-binding work; the cleanup and its comment belong to the
throwing-postcondition work.  Cut at the comment.  The body itself is not cut
further: its retval stand-in prologue, the P3098 capture-flag gating loop and
the copy-back epilogue are interleaved, and the closing brace of the function
sits at the end of the epilogue, so any interior cut leaves one piece with an
unbalanced brace -- but for D4405's override shadows' obligations of each
postcondition, ahead of its check (`4940-p4405-override`), a statement of
their own: cut around them.

- +/^      \/\* D4405: its obligations in the override shadows, ahead of it\.  \*\/$/
- +/^      \/\* If this postcondition has captures, gate on the initialized flag\.  \*\/$/
- +/^\/\* Wrap STMTS -- the postcondition checks of FNDECL/

## gcc/cp/contracts.cc#101

The non-exceptional path of `maybe_apply_function_contracts`'s
postcondition checks (`4000-p3098`), behind D4405's override shadows'
obligations of the overridden functions' postconditions, after the
function's own checks (`4940-p4405-override`).  Cut after the latter.

- +/^      post_stmts = pop_stmt_list \(post_stmts\);$/

## gcc/cp/contracts.cc#103

The postcondition checks of a function that cannot throw
(`4000-p3098`), with D4405's override shadows' obligations of the
overridden functions' postconditions after the function's own checks
(`4940-p4405-override`).  Cut around the latter.

- +/^      add_override_base_post_obligations \(\);$/
- +/^      post_stmts = pop_stmt_list \(post_stmts\);$/

## gcc/cp/contracts.cc#221.12
The tail of `contract_local_handler_always_rethrows_p` runs straight into the
documentation comment of P3100's `build_contract_assumption` -- anchors.py
cannot see the boundary because the comment's continuation lines are
indented text, not `*`-prefixed, so it does not read as a comment head.
Cut at the comment so the function and its documentation travel together.

- +/^\/\* The evaluation of CONTRACT with the P3100 assume semantic: an optimizer$/

## gcc/cp/contracts.cc#13.26

D4405's `contract_diagnose_lambda_conversion`, then the comment of D4405's
constant-evaluation sentinel, which goes with it.  Cut at that comment.

- +/^\/\* D4405 \("The Analysis Evaluation"\): there is no analysis evaluation$/

## gcc/cp/contracts.cc#13.42

D4405's `contract_never_evaluated_assertion`, then the comment of P4405's
`diagnose_caller_side_proving`, which goes with its function.  Cut at that
comment.

- +/^\/\* P4405: diagnose the analysis evaluations of a call of CALLEE with the$/

## gcc/cp/contracts.cc#221.13

P3100's `build_contract_assumption`, then the comment of D4405's
`contract_analyzed_here_p`, which goes with its function.  Cut at that
comment.

- +/^\/\* D4405: true if ANALYSIS, the semantic of CONTRACT's analysis$/

## gcc/cp/contracts.cc#221.15

D4405's `build_analysis_evaluation`, then the comment of P3595's
`emit_check_for_semantic`, which goes with its function.  Cut at that
comment.

- +/^\/\* Emit the check body for CONTRACT under a single, statically known$/

## gcc/cp/contracts.cc#130
The start of `maybe_contract_wrap_call`: `2400-p3595` replaces upstream's
`handle_contracts_p` test with `handle_caller_contracts_p`, and
`4900-p4405` puts its inheriting-constructor hook in front of it.  Cut at
the test and send upstream's removed line with its replacement.

- +/^  if \(!handle_caller_contracts_p \(fndecl\)\)$/ -/^  if \(!handle_contracts_p \(fndecl\)\)$/

## gcc/cp/contracts.cc#229
One hunk removes the violation-handler-wrapper preamble that
`2200-libcontracts` replaces with `__cxa_` entry points, and
adds two unrelated things: P3595's `unshare_expr` of the condition, needed
because the emitter is now called once per dynamic-dispatch arm, and the
`kind` local that `declare_cxa_entry_point` takes.  Cut before `kind`, and
send every removed line to piece 1 -- the removals are the libcontracts
rewrite, not the unshare.

- +/^  \/\* Determine the assertion kind for entry point selection\./ -/^  if \(calls_handler\)$/

## gcc/cp/contracts.cc#232
The violation-data emission.  `2200-libcontracts` replaces the
`contract_violation` object with a data block; P3595 then wraps that in
`if (shared_data_addr) ... else { ... }` so every arm of one dynamic
dispatch reuses a single block instead of emitting one global per arm.  The
two are interleaved: the P3595 lines are the opening five and, six lines
later, the brace that closes the `else`.  Three cuts, giving four
alternating pieces, are what it takes -- and cutting this finely rather than
stopping at the first boundary is exactly what keeps both halves
brace-balanced.  Pieces 1 and 3 close the `if (!quick && calls_handler)`
block the base file opens, and pieces 0 and 2 are a complete if/else on
their own.  All five removed lines are the libcontracts rewrite and go to
piece 1.

- +/^\t  \/\* Build a data block for the violation\./ -/^      \/\* Build a violation object, with the contract settings\./
- +/^\t\}$/
- +/^    \}$/

## gcc/c-family/c-cppbuiltin.cc#1

The per-paper feature-test macros: eleven `if (flag_contracts_pNNNN)` /
`cpp_define (...)` pairs in one contiguous insertion, with no unchanged line
between them.  Strictly line-per-paper, so cut on each `if` and let every
paper define its own macro.  P4299's comes after P4301's, with a comment of
its own, so cut on the comment.  The last pair is `-fcontracts-allow-assume`,
which belongs to `6000-p3100-core` along with the `assume` semantic itself.

- +/^      if \(flag_contracts && flag_contracts_p3290\)$/
- +/^      if \(flag_contracts && flag_contracts_p3400\)$/
- +/^      \/\* P4451 raises __cpp_contracts, which the library cannot key on/
- +/^      if \(flag_contracts && flag_contracts_p3098\)$/
- +/^      if \(flag_contracts && flag_contracts_p4283\)$/
- +/^      \/\* P3100, like P3290, specifies no language feature-test macro/
- +/^      if \(flag_contracts && flag_contracts_p4298\)$/
- +/^      if \(flag_contracts && flag_contracts_p4301\)$/
- +/^      if \(flag_contracts && flag_contracts_p4405\)$/
- +/^      \/\* P4299's keywords, _Pre, _Post and _ContractAssert/
- +/^      if \(flag_contracts && flag_contracts_allow_assume\)$/

## gcc/c-family/c-cppbuiltin.cc#2

The C side of the same macros: P4299's own vendor macro, then
`-fcontracts-allow-assume`'s, which is `6000-p3100-core`'s, then
`__gcc_contracts_violation_information_none`, defined in every language and
`6990-contract-violation-information`'s.  Cut at each one's comment.

- +/^  \/\* P4299 C contracts honour -fcontracts-allow-assume too\.  \*\/$/
- +/^  \/\* -fcontract-violation-information=none: <cassert>'s contract path and$/

## gcc/c-family/c.opt#3

Three `EnumValue` rows for `-fcontract-evaluation-semantic=`: `assume` from
P3100 and `noexcept_observe` / `noexcept_enforce` from P4298.

The cut lands ONE LINE LATE and cannot be made to land where it should.  A
row is two lines, a bare `EnumValue` marker and the `Enum(...) String(...)`
that follows it, and the three markers are byte-identical; a cut regex has
to match exactly one line of the segment, so `^EnumValue$` is rejected as
ambiguous.  Cutting on the first `Enum(...) String(noexcept_observe)`
instead leaves P4298's first `EnumValue` marker in piece 0, with P3100.  One
marker line attributed to the wrong paper is the cost; the alternative is
leaving all nine lines unattributed in a cross-paper commit.  The assembled
file is correct either way -- only the intermediate P4298 commit sees a
malformed fragment, which its `## Compile gap` records.

- +/^Enum\(contract_semantic\) String\(noexcept_observe\) Value\(6\)$/

## gcc/c-family/c.opt#4

The `-fcontract-evaluation-semantic=` option record and its help text.  The
record line is widened from `C++ ObjC++` to `C C++ ObjC ObjC++` by P4299;
the help line spells out all seven semantics, so it names P3100 (`assume`)
and P4298 (`noexcept_*`) in one string and cannot be divided.  Cut between
them; the two removed lines stay with piece 0, which is the commit that
rewrites the record.

- +/^-fcontract-evaluation-semantic=\[ignore\|observe\|enforce\|quick_enforce\|assume/

## gcc/c-family/c.opt#6

The `-fcontract-*` records that sort between `-fcontract-disable-optimized-checks`
and `-fcontracts-client-check=`, in collating order: P3400's
`-fcontract-bypass-rethrowing-local-handler`, the base facility's
`-fcontract-comment-from-expression`, P3595's configuration and group
options, `-fcontract-violation-information=` and its `Enum`, and P3100's
`-fcontracts-allow-assume`.  Cut before each commit's first record.

- +/^fcontract-comment-from-expression$/
- +/^fcontract-configuration=$/
- +/^Enum$/
- +/^fcontracts-allow-assume$/

## gcc/cp/g++spec.cc#3

The driver's copy of the option list: one `case OPT_fcontracts_pNNNN:` label
per paper, then the comment explaining the whole block.  The label run is
strictly line-per-paper, so cut it that way.  The comment is the reason the
block exists at all -- linking the contracts runtime -- and stays with
`2200-libcontracts`, as do the lines recording the value of `-fcontracts`
(before the first paper label) and each per-paper flag's last value (after
the last one).

- +/^\tcase OPT_fcontracts_p3097:$/
- +/^\tcase OPT_fcontracts_p3098:$/
- +/^\tcase OPT_fcontracts_p3099:$/
- +/^\tcase OPT_fcontracts_p3100:$/
- +/^\tcase OPT_fcontracts_p3290:$/
- +/^\tcase OPT_fcontracts_p3400:$/
- +/^\tcase OPT_fcontracts_p3850:$/
- +/^\tcase OPT_fcontracts_p4283:$/
- +/^\tcase OPT_fcontracts_p4298:$/
- +/^\tcase OPT_fcontracts_p4301:$/
- +/^\tcase OPT_fcontracts_p4405:$/
- +/^\tcase OPT_fcontracts_p4451:$/
- +/^\t  \/\* Any use of C\+\+ contracts needs/

## gcc/c-family/c.opt.urls#0

The URLs of the three warnings that `c.opt#0` also declares in a row:
P3595's `-Wcontract-configuration`, P3400's `-Wcontract-invalid-label-facet`,
then P4405's `-Wcontract-unproven`.  Cut on the second and third option
names, as there.

- +/^Wcontract-invalid-label-facet$/
- +/^Wcontract-unproven$/

## gcc/c-family/c.opt.urls#2

The URLs of the options after `-fcontract-disable-optimized-checks`:
P3400's `-fcontract-bypass-rethrowing-local-handler`, the base facility's
`-fcontract-comment-from-expression`, P3595's three configuration options,
`-fcontract-violation-information=`, and P3100's `-fcontracts-allow-assume`.
Cut at each owner's first option.

- +/^fcontract-comment-from-expression$/
- +/^fcontract-configuration=$/
- +/^fcontract-violation-information=$/
- +/^fcontracts-allow-assume$/

## gcc/doc/invoke.texi#2

The option-summary synopsis lines after `-fcontract-disable-optimized-checks`:
P3400's `-fno-contract-bypass-rethrowing-local-handler`, then the base
facility's `-fcontract-comment-from-expression`, then
`-fcontract-violation-information=`.  Cut between them.

- +/^-fcontract-comment-from-expression$/
- +/^-fcontract-violation-information=/

## gcc/doc/invoke.texi#3

The option-summary synopsis.  Its first two lines are P3595's configuration
options, then P3100's `-fcontracts-allow-assume`, then three lines that pack
the per-paper flags four to a line, which `@gccoptlist` renders as a run-on
list.  A line is the smallest thing a cut can move, so the three per-paper
lines are indivisible and travel together to the cross-paper commit.

- +/^-fcontracts-allow-assume$/
- +/^-fcontracts-p3097  -fcontracts-p3098  -fcontracts-p3099  -fcontracts-p3100$/

## gcc/doc/invoke.texi#6

The C Dialect entry for `-fcontracts-p4299`, whose last paragraph says that
P3100's `-fcontracts-p3100` (and P4298's and P4405's options) enable it in
C.  Cut before that paragraph.

- +/^In C, each of @option\{-fcontracts-p3100\}, @option\{-fcontracts-p4298\} and$/

## gcc/doc/invoke.texi#7

The tail of the `-fcontracts` description: the `__pre`/`__post` extension
spellings, then the paragraph on running a program that depends on the
shared `libcontracts.so.1`, which names the per-paper options and
`-fcontracts-p4299` and so belongs to the cross-paper commit.  Cut at its
index entry.

- +/^@cindex @file\{libcontracts\.so\.1\}$/

## gcc/doc/invoke.texi#8

Paragraphs added to the `-fcontract-evaluation-semantic=` semantic list:
`assume` (P3100, and its `-fcontracts-allow-assume` gate), the two P4298
nonthrowing semantics, the two P4405 semantics, how an assertion-control
label (P3400) limits the semantic, and P4405's sentence on how that best fit
treats its two.  Cut between them.

- +/^'@code\{noexcept_enforce\}' As @code\{enforce\}/
- +/^'@code\{prove\}' The predicate is not evaluated/
- +/^The semantic selected for a contract assertion, by this option or by the$/
- +/^A disallowed @code\{prove\} becomes @code\{diagnose\}/

## gcc/doc/invoke.texi#10

The descriptions after `-fcontract-disable-optimized-checks`: P3400's
`-fno-contract-bypass-rethrowing-local-handler`, then the base facility's
`-fcontract-comment-from-expression`, then
`-fcontract-violation-information=`.  Cut at each one's first `@opindex`.

- +/^@opindex fcontract-comment-from-expression$/
- +/^@opindex fcontract-violation-information$/

## gcc/doc/invoke.texi#11.3

P3100's paragraph on the `ub:` groups of implicit contract assertions, then
P3595's `-fcontract-configuration` item headers.  Cut at the headers.

- +/^@opindex fcontract-configuration$/

## gcc/doc/invoke.texi#11.14

The end of P3100's `-fcontracts-allow-assume` description, then the item
header of `-fcontracts-p3850`.  Cut at the header.

- +/^@opindex fcontracts-p3850$/

## gcc/doc/invoke.texi#11.21

The paper roll-call: the tail of the option rule appended to the
`-fcontracts-p3850` description, then eleven `@opindex fcontracts-pNNNN` /
`@opindex fno-contracts-pNNNN` pairs, then the eleven `@item` / `@itemx`
lines they index.  Every one is per paper, so cut at each; each paper claims
its own pair and its own `@item`.  Piece 0, the tail of the option rule, is
`6900-cross-paper-options`.

- +/^@opindex fcontracts-p3097$/
- +/^@opindex fcontracts-p3098$/
- +/^@opindex fcontracts-p3099$/
- +/^@opindex fcontracts-p3100$/
- +/^@opindex fcontracts-p3290$/
- +/^@opindex fcontracts-p3400$/
- +/^@opindex fcontracts-p4283$/
- +/^@opindex fcontracts-p4298$/
- +/^@opindex fcontracts-p4301$/
- +/^@opindex fcontracts-p4405$/
- +/^@opindex fcontracts-p4451$/
- +/^@item -fcontracts-p3097$/
- +/^@itemx -fcontracts-p3098$/
- +/^@itemx -fcontracts-p3099$/
- +/^@itemx -fcontracts-p3100$/
- +/^@itemx -fcontracts-p3290$/
- +/^@itemx -fcontracts-p3400$/
- +/^@itemx -fcontracts-p4283$/
- +/^@itemx -fcontracts-p4298$/
- +/^@itemx -fcontracts-p4301$/
- +/^@itemx -fcontracts-p4405$/
- +/^@itemx -fcontracts-p4451$/

## gcc/c-family/c-opts.cc#0

The includes: P3595's configuration headers (`2400-p3595`), then the
proving interface's header and the proof shadows' middle-end header, for
the hook the proof optimization level sets (`4950-p4405-prove-optimize`).
Cut before the latter.

- +/^#include "c-family\/contracts-prove\.h"$/

## gcc/c-family/c-opts.cc#6

`c_common_post_options`' warnings for an extension semantic named without
its flag: P4298's two, then P4405's two.  Cut before P4405's.

- +/^  \/\* Likewise the P4405 prove and diagnose semantics, which need$/

## gcc/doc/invoke.texi#12

The two warnings in Warning Options: P3595's `-Wcontract-configuration`,
whose second paragraph is P4298's (the noexcept semantics it reports) and
ends in P4405's sentence (prove and diagnose), then P3400's
`-Wcontract-invalid-label-facet`.  Cut at each.

- +/^The warning also reports an entry asking for the @code\{noexcept_enforce\}$/
- +/^It also reports an entry asking for the @code\{prove\} or @code\{diagnose\}$/
- +/^@opindex Wcontract-invalid-label-facet$/

## libstdc++-v3/include/std/contracts#0

The `__glibcxx_want_*` requests for the contracts feature-test macros this
branch adds, in `version.def` order: P3290's four (`message`, `api`,
`implicit`, `labels` arrive with it), then P4301's `report`.  Cut before
`report`.

- +/^#define __glibcxx_want_contracts_report$/

## libstdc++-v3/include/std/contracts#6

The extension `evaluation_semantic` enumerators: P3100's `assume`, P4298's
`noexcept_observe` / `noexcept_enforce`, then P4405's `prove` / `diagnose`.
Cut before `prove`.

- +/^    prove = 8, \/\/ P4405$/

## libstdc++-v3/include/std/contracts#12

P3400's `is_nonthrowing`, whose body opens with P4405's two semantics.
Cut that `if` out, again where P3400's `return` resumes, and before
P4451's terminating label, which follows the base facility's
`invoke_default_contract_violation_handler`.

- +/^    \/\/ P4405: no run-time effect, so nothing to throw\.$/
- +/^    return __s == evaluation_semantic::ignore$/
- +/^#if defined\(__gcc_contracts_p4451\)/

## libstdc++-v3/include/std/contracts#4

The `assertion_kind` enumerators of the unadopted papers, each group gated
on its paper's feature-test macro: P3290's `manual` and `cassert`, then
P3098's `post_capture`, then `implicit`, which arrived with P3290's group.
Cut at P3098's comment and again at `implicit`.

- +/^    \/\/ P3098's only with postcondition captures, likewise\.$/
- +/^    implicit = 7, /

## libstdc++-v3/include/bits/version.h#1

The generated blocks for the same macros, in the same order; cut before
P4301's `contracts_report` block.

- +/^#if !defined\(__cpp_lib_contracts_report\)$/

## libstdc++-v3/include/std/contracts#9

`evaluation_semantic_set`, the classification functions and
`violation_handled` (P3400's library surface) run straight on into the
`contract_violation` class head and its ABI-chain member, which replace the
old data members (the libcontracts rework); the diff has no unchanged line
between them.  Cut at the class head, which goes with the removed data
members.

- +/^  class contract_violation \{$/ -/^  class contract_violation \{$/

## libstdc++-v3/include/std/contracts#11

The `contract_violation` member-declaration list.  The whole run replaces the
old inline accessors that read the `_M_comment`/`_M_src_loc_ptr` members with
declaration-only members that read the ABI chain instead -- that is the
libcontracts rework, and it is what `2200-libcontracts`'s own out-of-line
`comment()` definition needs to have been declared.  Sitting in the middle of
it, guarded by `#if defined(__cpp_lib_contracts_report)`, is P4301's
`report()` declaration, which is the one member of the list the ABI rework
does not require.  Two cuts, giving three pieces: the accessors before
`report()`, the guarded `report()` block, and the accessors after it.  All
seven removed lines are the old inline bodies and stay with piece 0.

- +/^#if defined\(__cpp_lib_contracts_report\)$/
- +/^    std::source_location location\(\) const noexcept;$/

## libstdc++-v3/src/c++26/contract26.cc#0.1

`__handle_contract_violation_default`, the default violation handler.  It is
`2200-libcontracts`' code, but its first act is to print
`violation.report()` -- and `report()` is the entire content of P4301, added
2,700 bands later.  Without a cut, either the handler drags P4301's member
forward or `2200` calls a function that does not exist yet.

Two cuts, giving three pieces: the handler's opening through the
`_GLIBCXX_VERBOSE` guard, the three-line `report()` stanza, and the rest of
the body.  Only the middle piece belongs to `5200-p4301`; a reader of `2200`
sees a handler that never mentions reports, which is exactly right at that
point in the history.

- +/^  if \(const char\* __r = __v\.report\(\); __r && __r\[0\] != '\\0'\)$/
- +/^  \/\/ A location part or comment the violation leaves empty is left out\.$/
- +/^    case std::contracts::evaluation_semantic::noexcept_observe:$/
- +/^    case std::contracts::evaluation_semantic::prove:$/
- +/^    default:$/#2

The handler's semantic switch adds one `case`/print/`break` triple per
evaluation semantic, and two of them -- `noexcept_observe` and
`noexcept_enforce` -- are P4298's.  They are contiguous and each triple is
self-contained, so the third and fourth cuts hand exactly those six lines to
`4800-p4298`.  Dropping them leaves valid code with correct behaviour: the
`default:` arm already prints `unknown(N)` for a semantic it does not name,
which is what a history that has not yet added those semantics should say.
The two triples after them, `prove` and `diagnose`, are P4405's, and the
fifth cut hands them to `4900-p4405` the same way.

## libstdc++-v3/src/c++26/contract26.cc#0.12

`contract_violation::is_terminating()`, whose whole body is a three-term
disjunction -- and the third term is P4298's `noexcept_enforce`.  One cut
gives that term to `4800-p4298`.

The term is the last line of the return statement and therefore carries the
`;`, so `2200-libcontracts` is left with an unterminated expression.  That
is a genuine one-character scaffold rather than something the cut can avoid:
the semicolon belongs to the statement, not to the term, and no line
boundary separates them.  `2200`'s gap records it and `4800-p4298`
overwrites it.

- +/^      \|\| __s == evaluation_semantic::noexcept_enforce;$/
- +/^}$/

## gcc/cp/constexpr.cc#12.3

`constexpr_global_ctx::note_modification`, `1020-constexpr-side-effect`'s,
runs straight into the documentation comment of `contract_report_tracker`,
which is `1025-contract-discardable-evaluation`'s.  Cut at the comment.

- +/^\/\* Helper class for constexpr_global_ctx\.  The operand of an \[\[assume\]\] is$/

## gcc/cp/constexpr.cc#35

The contract-statement case's evaluation of the predicate under a
`predicate_rollback`, `1020-constexpr-side-effect`'s, carries
`3610-constexpr-contract-evaluation`'s handling of a predicate that exits via
an exception: the declarations of what to report and of the heap watermark
just after `tree eval;`, and, inside the rollback's scope, the discarding of
the exception.  Cut around both.

- +/^\ttree exc_report = NULL_TREE;$/
- +/^\t\{$/
- +/^\t  \/\* Before the rollback, while the exception object still has its$/
- +/^\t\}$/

## gcc/cp/contracts.cc#2

The includes P3100's sanitizer routing needs (`varasm.h`, `ubsan.h`) run on
into `predict.h`, which the branch hint on the violation arm of a check
needs.  Cut before `predict.h`.

- +/^#include "predict\.h"$/

## gcc/cp/contracts.cc#124
`update_contract_arguments`' comment ends with P3595's note that the
non-deferred path copies and remaps more than it needs, and runs straight on
into the warning that the deferred path's copy must stay unconditional --
there is no unchanged line between them.  The first belongs to the
configuration work that wrote the remap; the second to the declarator-position
change, which is what made every contract deferred and so made that copy
load-bearing.  Cut between them.

- +/^    The copy is unconditional on purpose, and it is tempting to think it$/

## gcc/cp/contracts.cc#13.2

`dynamic_dispatch_semantics` (P3595 dynamic dispatch) lists the four C++26
semantics, and the P4298 noexcept pair under `-fcontracts-p4298`; cut the
P4298 block out so it can go with P4298.

- +/^  if \(flag_contracts_p4298\)$/
- +/^  return v;$/

## gcc/cp/contracts.cc#20.7

P3100's `cp_resolve_implicit_ub_semantic` runs on into the documentation
comment of `group_name_type`, which belongs to the P3400 facet framework
with the function it describes.  Cut at the comment.

- +/^\/\* The type a group name is read as: std::string_view, as$/

## gcc/cp/contracts.cc#20.2

`contract_base_allowed_mask` (P3400's allowed-semantics facet) gates each
extension semantic on its flag; the P4405 pair under `-fcontracts-p4405` is
the last gate.  Cut it out for P4405, and again at the `return` that ends
the function.

- +/^  if \(flag_contracts_p4405\)$/
- +/^  return mask;$/

## gcc/cp/contracts.cc#13.15

`all_contracts_statically_nonthrowing` (P4298), then the comment of P3100's
`contract_lowered_as_assumption_p`, which anchors.py leaves behind (its
continuation lines do not read as a comment).  Cut at the comment.

- +/^\/\* True if an evaluation of CONTRACT with the semantic SEM is lowered as an$/

## gcc/cp/contracts.cc#13.16

`contract_lowered_as_assumption_p` (P3100), then the comment of P3595's
`contract_active_p`.  Cut at the comment.

- +/^\/\* Return true if CONTRACT is checked or assumed under the current build$/

## gcc/cp/contracts.cc#13.17

`contract_active_p` (P3595's activity query) resolves the run-time
semantic, then P3100 keeps an `assume` evaluation, which is an optimizer
assumption, in the body, then the dynamic-selection forcing.  Cut P3100's
statement out.

- +/^  \/\* P3100: an assume evaluation is an optimizer assumption in the body\.  \*\/$/
- +/^  \/\* A dynamic contract is always active at run time regardless of its$/

## gcc/cp/contracts.cc#138

`define_one_contract_wrapper_func` (P3097) decides whether a non-virtual
wrapper checks the postconditions; P3100's test that one is an `assume`
evaluation, which is an optimizer assumption, comes before the P3595
dynamic-selection test.  Cut P3100's test out.

- +/^\t  \/\* P3100: an assumption, as in maybe_contract_wrap_call\.  \*\/$/
- +/^\t  \/\* A dynamic postcondition \(descriptor present\) must be checked even$/

## gcc/cp/contracts.cc#132

`maybe_contract_wrap_call` (P3595 caller-side checks) resolves the caller
semantic tuple and decides whether any entry is active; P4405's interface
places the markers of evaluations a proving strategy deferred, through
variables before and an early return after; D4405's analysis evaluations
at the call (each precondition's, and what a caller-side prove or diagnose
gives a contract) are diagnosed between them, around the tuple's
resolution; and P3100's loop makes a caller-side assume build the wrapper.
Cut each out.

- +/^  \/\* P4405 proving hook: the markers of deferred caller-side evaluations,$/
- +/^  \/\* D4405: the analysis evaluations at this call -- each precondition's,$/
- +/^  if \(!is_virtual\)$/#1
- +/^  tree specs = get_fn_contract_specifiers \(DECL_ORIGIN \(fndecl\)\);$/
- +/^  if \(!is_virtual\)$/#2
- +/^      \/\* P3100: a caller-side assume is an optimizer assumption, made in the$/
- +/^      for \(tree t = tuple; t; t = TREE_CHAIN \(t\)\)$/
- +/^  \/\* P4405 proving hook: the markers come before the call or the wrapper$/
- +/^  if \(!any_active\)$/

## gcc/cp/contracts.cc#223

`emit_check_for_semantic`'s switch: P3100's `assume` arm, sharing ignore's
`return void_node`, then P4405's `prove` / `diagnose` arm, which diagnoses
the evaluation.  Cut out the assumption the `assume` arm builds before that
`return`, and cut at P4405's arm.

- +/^      \/\* P3100: assume, which never evaluates the predicate either, is an$/
- +/^      return void_node;$/#1
- +/^    case CES_PROVE:$/

## gcc/cp/contracts.cc#13.13

P4405's `add_contract_proof_assumption`, the last of the helpers after
`diagnose_unproven_contract`, then the comment of
`contract_semantic_is_nonthrowing`, which stays with the P3595 piece it was
in before P4405 put its functions in between.  Cut at that comment.

- +/^\/\* True if SEM never permits an exception to escape a handler call: the$/

## gcc/cp/contracts.cc#240.70

`finish_contract_control_expr` (P3400 core) runs on into the comment of
`contract_evaluation_settled_p`, which belongs to the refinable-body
settledness rule (allowed_semantics).  Cut at that comment.

- +/^\/\* P3400: true if the evaluation of CONTRACT, a contract assertion of$/

## gcc/cp/contracts.cc#240.69

`build_runtime_contract_check`, the runtime evaluation `build_contract_check`
emits after the analysis evaluation (P3595 dynamic dispatch), hands a
dynamically selected postcondition with captures to
`build_dynamic_capture_selection` (P3098) before building the dispatch; cut
that hook out so it can go with P3098.  Before both, P4405's interface
makes the analysis evaluations of the calls in a predicate that a check
evaluates, whose markers the check's copy of the predicate carries, and
emits a plain
check followed by what lets a proving strategy that defers evaluations
assume the predicate; cut that out for P4405, around D4405's early return
for an ignore evaluation after an analysis evaluation, and again at the
plain check that follows it.

- +/^  \/\* P4405 proving hook: the calls in a predicate that a check evaluates,$/
- +/^  \/\* D4405: an ignore evaluation after a prove or diagnose analysis$/
- +/^  \/\* P4405 proving hook: a plain check, followed by what lets a strategy$/
- +/^  if \(!dyn_name\)$/
- +/^  \/\* A postcondition with captures chooses its semantic once, where the$/
- +/^  \/\* P3595 dynamic selection\.  Dispatch on the selector's runtime return value,$/

## gcc/cp/contracts.cc#21.4

The declaration of `contract_decl_map`, rewritten to hash its keys in a way a
precompiled header preserves, runs on into the documentation comment of
`contract_this_is_const_p`, which belongs to the `this`-is-const work.  Cut at
the comment.

- +/^\/\* True if a use of .this. at the current point is in a contract predicate,$/

## gcc/cp/parser.cc#86

P4299's `_Pre` and `_Post` keywords, inserted at the top of the P4283
rewrite of `cp_function_contract_specifier_intro`.  Cut between the comment
and the code; both pieces are P4299's.

- +/^  if \(token->keyword == RID_C_PRE\)$/

## gcc/cp/parser.cc#88.2

`cp_nth_token_is_contract_intro_p` is the P4283 lookahead; the two lines
that accept P4299's `_Pre` and `_Post` keywords in it are P4299's.

- +/^  if \(tok->keyword == RID_C_PRE \|\| tok->keyword == RID_C_POST\)$/
- +/^  return \(tok->type == CPP_NAME$/

## gcc/cp/contracts.h#15

The postcondition declarations, then P3098's capture helpers, then the
declaration of `member_template_spec_contracts` (explicit
specializations).

- +/^extern bool postcondition_capture_init_ok_p/
- +/^extern void member_template_spec_contracts/


## gcc/cp/pt.cc#10
`tsubst_contract` applies [dcl.contract.func]/7 to the substituted
postcondition (the odr-use rework) and, in the same run of added lines,
judges the declarations merged away against what that walk found (the
const rule across redeclarations).  Cut before the second.

- +/^  \/\* That walk can be the first to find a parameter odr-used, so judge the$/

## gcc/cp/contracts.cc#169

`grok_contract`'s comment, now built from the text the parser formed for
it, ahead of the P3400 compute_comment facet.  Cut at the blank line.

- +/^$/

## gcc/cp/contracts.cc#173

The same in `update_late_contract`, ahead of the facet.  Cut at the blank
line.

- +/^$/

## gcc/cp/parser.cc#72

In `cp_parser_late_contract_condition`, the comment formed from the
predicate's tokens, ahead of P3099's message.  Cut at the blank line.

- +/^$/#1

## gcc/cp/parser.cc#82

In `cp_parser_contract_assert`, P3098's rename of the postcondition flag,
then the first token of the predicate, kept for its comment.

- +/^  cp_token \*first = cp_lexer_peek_token \(parser->lexer\);$/

## gcc/cp/parser.cc#83

The comment of a `contract_assert`, ahead of P3099's message.  Cut at the
blank line.

- +/^$/#1

## gcc/cp/parser.cc#107

In `cp_parser_function_contract_specifier`, the comment of a contract
parsed in place, then P3099's message and the extension fields' call to
`grok_contract`, which replaces upstream's, then P4451's marking of a
terminating assertion, then the extension fields' P3098 captures.

- +/^      tree message = cp_parser_contract_message \(parser\);$/ -/^      \/\* Build the contract\.  \*\/$/
- +/^      if \(terminating && contract != error_mark_node\)$/
- +/^      \/\* Store captures on the postcondition node\.  \*\/$/

## gcc/cp/init.cc#3

`build_new_1` checks the result of a non-allocating placement new for null
(`6360-p3100-check-placement-new-null`) and then for alignment where the
new-expression stores nothing (`6370-p3100-check-placement-new-alignment`),
in one run of added lines.  Cut at the alignment check.

- +/^  \/\* P3100: and it must be suitably aligned for the object created there$/

## gcc/cp/pt.cc#9

`tsubst_contract` substitutes a contract (contract substitution plumbing)
and, at the end of the same run of added lines, marks a constructor
precondition or destructor postcondition for the P3100 check of member
functions called on `*this` (`6400-p3100-check-base-init-mem-fun`).  Cut
before the second.

- +/^  \/\* P3100: and check a member function called on \(\*this\) in a$/

## gcc/cp/call.cc#1

`keep_unused_object_arg`: the P3100 null check of an object `*p` whose
static member function is called (`6110-p3100-check-null-and-alignment`)
sits inside the postcondition odr-use change that splits the function's
early return (`1060-postcondition-odr-use`).  Cut it out.

- +/^  \/\* P3100: an indirection through a pointer, `\*p', is evaluated too; the$/
- +/^  if \(!TREE_SIDE_EFFECTS \(obj\)\)$/

## gcc/cp/call.cc#7

`p3100_member_call_checks` makes both lexical member-call checks of a
member function called on `*this`: cut out the base-initializer check
(`6400-p3100-check-base-init-mem-fun`) from the function-try-block handler
check (`6390-p3100-check-handler-member`) that introduces the helper.

- +/^  call = \(build_implicit_this_operation_check$/
- +/^  return build_implicit_handler_member_check \(input_location, object, call\);$/

## gcc/cp/typeck.cc#38

In `build_static_cast_1`'s reference downcast, the P3100 marking of the
derived object's address (`P3100_DOWNCAST_ADDRESS_P`) runs straight into
the `-fsanitize=vptr` check of the reference form, which an upstream
bugfix moved after the conversion, with no unchanged line between.  Cut at
the blank line before the vptr check (the marking has none inside it) so
each travels with its own commit, the blank line with the vptr check's.

- +/^$/

## gcc/c-family/c-ubsan.h#1

The declaration of `p3100_flexible_array_member_like_p` (`6220-p3100-check-bounds`)
is followed by upstream's `ubsan_maybe_instrument_reference`, whose new
location parameter (`6110-p3100-check-null-and-alignment`) joins the two
into one hunk.  Cut at the changed declaration.

- +/^extern void ubsan_maybe_instrument_reference \(tree \*,$/ -/^extern void ubsan_maybe_instrument_reference \(tree \*\);$/

## gcc/Makefile.in#1

`C_COMMON_OBJS`'s fork additions: P3595's `contracts-config.o`
(`2400-p3595`), then P4405's proving dispatcher `contracts-prove.o`
(`4900-p4405`), its proof shadow strategy `contracts-prove-shadow.o`
(`4920-p4405-shadow`), its override shadow strategy
`contracts-prove-override.o` (`4940-p4405-override`) and its folding
strategy `contracts-prove-fold.o` (`4910-p4405-fold`), one object to a line
so that each lands with its own source file.

- +/^  c-family\/contracts-prove\.o \\$/
- +/^  c-family\/contracts-prove-shadow\.o \\$/
- +/^  c-family\/contracts-prove-override\.o \\$/
- +/^  c-family\/contracts-prove-fold\.o$/

## gcc/cp/contracts.cc#0

The fork's includes: `cp/contracts-config.h` and `cp-objcp-common.h`
(`2400-p3595`), then P4405's `c-family/contracts-prove.h` (`4900-p4405`).
Cut before the last.

- +/^#include "c-family\/contracts-prove\.h"$/

## gcc/c-family/contracts-prove.cc#0.5

The registry of proving strategies (`4900-p4405`), whose entries are the
folding strategy (`4910-p4405-fold`), the premise copies and the proof
shadows (`4920-p4405-shadow`) and the override shadows
(`4940-p4405-override`).
Cut around each entry; the terminating null pointer and the closing brace
are the registry's.

- +/^  &contract_prove_fold_strategy,$/
- +/^  &contract_prove_callee_premises_strategy,$/
- +/^  &contract_prove_shadow_strategy,$/
- +/^  &contract_prove_override_strategy,$/
- +/^  NULL$/

## gcc/c-family/contracts-prove.h#0.28

The declaration of the premise copies' strategy (`4920-p4405-shadow`),
then the documentation of the override shadows' strategy
(`4940-p4405-override`), whose declaration starts the next piece.  Cut at
the latter.

- +/^\/\* -fcontracts-prove-override \(contracts-prove-override\.cc\): the override$/

## gcc/c-family/contracts-prove.h#0.30

The declaration of the override shadows' predicate
(`4940-p4405-override`), then the documentation of the folding strategy's
front-end hook (`4910-p4405-fold`), whose declaration starts the next
piece.  Cut at the latter.

- +/^\/\* Defined by each front end, for the folding strategy: EXPR folded as far$/

## gcc/c-family/contracts-prove.h#0.33

The declaration of the second of the proof shadows' diagnostic-context
hooks (`4920-p4405-shadow`), then the header's closing `#endif`
(`4900-p4405`).

- +/^#endif \/\* GCC_C_FAMILY_CONTRACTS_PROVE_H \*\/$/

## gcc/doc/invoke.texi#11.28

The end of P4451's paragraph after the per-paper roll-call
(`5400-p4451`), then the item of the P4405 proving strategies,
`-fcontracts-prove-fold` and `-fcontracts-prove-assume` (`4910-p4405-fold`).
Cut at the item.

- +/^@opindex fcontracts-prove-fold$/

## gcc/doc/invoke.texi#11.12

The end of the caller-side precondition rule appended to P3595's
`-fcontract-configuration` description (`2400-p3595`), then P3100's item
header of `-fcontracts-allow-assume`.  Cut at the header.

- +/^@opindex fcontracts-allow-assume$/

## gcc/doc/invoke.texi#11.62

The end of the item of `-fcontracts-prove-shadow` (`4920-p4405-shadow`),
then the item of `-fcontracts-prove-override` (`4940-p4405-override`).  Cut
at the latter.

- +/^@opindex fcontracts-prove-override$/

## gcc/doc/invoke.texi#11.65

The end of the item of `-fcontracts-prove-override`
(`4940-p4405-override`), then the item of
`-fcontracts-prove-callee-premises` (`4920-p4405-shadow`).  Cut at the
latter.

- +/^@opindex fcontracts-prove-callee-premises$/

## gcc/doc/invoke.texi#11.36

The end of the item of the first P4405 proving strategies
(`4910-p4405-fold`), then the item of `-fcontracts-prove-shadow`
(`4920-p4405-shadow`).  Cut at the latter.

- +/^@opindex fcontracts-prove-shadow$/

## gcc/doc/invoke.texi#1

The C++ dialect option synopsis changes two adjacent lines for unrelated
reasons: the list of evaluation semantics grows (cross-paper options), and
`-fcontracts-conservative-ipa` goes (refinable bodies), which leaves
`-fcontract-checks-outlined` on a line of its own.  The removed synopsis
line is a declared removal (removals.md); cut the remaining hunk at the
new line, so that each commit adds its own.  D4405's
`-fcontract-analysis-semantic=` comes first, on a line of its own; cut
after it, taking the replaced semantics line with the new one.

- +/^-fcontract-evaluation-semantic=/ -/^-fcontract-evaluation-semantic=/
- +/^-fcontract-checks-outlined$/

## gcc/cp/cp-gimplify.cc#6
`mark_contract_wrapper_caller` is `0430-refinable-body-contracts`'s; the
early return for a P3097 wrapper whose evaluations are all settled is
`4230-p3400-facet-allowed-semantics`'s.  Cut around it.

- +/^  \/\* P3097: a wrapper for a virtual call whose evaluations are all$/
- +/^  mark_refinable_body \(current_function_decl\);$/

## gcc/cp/cp-gimplify.cc#11
`0170-postcondition-undeduced-result-ice` rewrote the contract case's
`if`; the branch for a settled evaluation between its two arms is
`4230-p3400-facet-allowed-semantics`'s.  The removed upstream line stays
with piece 0.

- +/^      \/\* P3400: an evaluation that every translation unit makes alike does$/
- +/^      else if \(tree check = build_contract_check \(stmt\)\)$/

## gcc/cp/decl.cc#19
`0430-refinable-body-contracts` marks a function with contract
specifiers; the clause leaving one whose specifiers are all settled
unmarked is `4230-p3400-facet-allowed-semantics`'s.

- +/^      && !contract_specifiers_settled_p \(fndecl\)$/
- +/^      && DECL_HAS_CONTRACTS_P \(fndecl\)\)$/

## libstdc++-v3/include/std/contracts#13.8

The `labels` namespace (P3400) holds three D4405 additions: the
`analysis_semantic_label` concept, after P3400's facet concepts, the
standard labels `prove`, `diagnose`, `always_prove` and `always_diagnose`,
after the `never_X` labels, and the combined label's `analysis_semantic`,
before its `compute_comment`.  Cut each out.

- +/^  \/\/ D4405: an assertion-control object can strengthen the semantic of the$/
- +/^  \/\/ P3400 \[support\.contract\.control\.general\]: a variable template for each$/
- +/^#ifdef __cpp_contracts_prove$/
- +/^  struct review_t \{$/
- +/^    \/\/ D4405: the stronger of the constituents' analysis semantics \(ignore,$/
- +/^    constexpr const char\*$/#1

## gcc/cp/contracts.cc#240.73

P3400's `contract_wrapper_settled_p` is followed, with no definition line
between, by the documentation comments of D4405's override shadows and of
`collect_overridden_functions`, which anchors.py leaves with the function
before them.  Cut at the first.

- +/^\/\* D4405R0, "Virtual Functions": override shadows\.  A call through a base$/

## gcc/cp/contracts.cc#220.12

`build_contract_data_block_ctor`, `2200-libcontracts`' builder of the
violation data block, with the block that empties its location, comment
and message under `-fcontract-violation-information=none` just before the
constructor is built.  Cut that block out for
`6990-contract-violation-information`.

- +/^  \/\* -fcontract-violation-information=none: the record carries an empty$/
- +/^  \/\* Build the constructor\.  \*\/$/

## gcc/c/c-parser.cc#18.28

`c_emit_contract_check`, P4299's emitter of a C contract check, with the
block that empties the violation's location and comment under
`-fcontract-violation-information=none` after its opening lambda.  Cut
that block out for `6990-contract-violation-information`.

- +/^  \/\* -fcontract-violation-information=none: the violation carries an empty$/
- +/^  if \(semantic != -1\)$/

## gcc/ginclude/contracts.h#0.25

The P3290 C API of `<contracts.h>`, `2200-libcontracts`', with the
redefinitions of its convenience macros under
`__gcc_contracts_violation_information_none` after them.  Cut those out for
`6990-contract-violation-information`; the closing `extern "C"` block
stays with `2200`.

- +/^\/\* -fcontract-violation-information=none: an empty location and comment, as$/
- +/^#ifdef __cplusplus$/

## libstdc++-v3/include/bits/assert_contract.h#0.2

P3290's contract-checking `assert`, with its redefinition under
`__gcc_contracts_violation_information_none` after it.  Cut that out for
`6990-contract-violation-information`; the closing `#endif`s stay with
`3300-p3290`.

- +/^#ifdef __gcc_contracts_violation_information_none$/
- +/^#endif \/\/ !NDEBUG && __STDC_WANT_ASSERT_USES_CONTRACTS__$/

## libstdc++-v3/include/std/contracts#13.0

The opening of P3290's library API, with the default location of a manual
violation, and its empty redefinition under
`__gcc_contracts_violation_information_none` after it.  Cut that out for
`6990-contract-violation-information`.

- +/^#ifdef __gcc_contracts_violation_information_none$/

## gcc/c-family/c-cppbuiltin.cc#0

`__cpp_contracts` is the greatest value any enabled paper gives it: P3097's
block computes it and raises it to its private date, and P4451's comment and
`if` between that and the `cpp_define` raise it to D4451R0's value.

- +/^\t  \/\* D4451R0's wording gives P4451's value as 202611L/
- +/^\t  char buf\[sizeof \("__cpp_contracts="\)/

## gcc/cp/contracts.h#3

Upstream's `CONTRACT_CONDITION` definition, moved by the operand
reordering, is deleted where P4451's `CONTRACT_TERMINATING_P` is added.

- +/^\/\* True if NODE is a terminating contract assertion \(P4451\): one whose$/

## gcc/cp/contracts.cc#9

`mismatched_contracts_p`'s comparisons of the extension fields: P3099's
message, then P4451's terminating flag, then P3400's label and the rest.

- +/^  \/\* Both are terminating contract assertions, or neither is \(P4451\): both$/
- +/^  \/\* Compare assertion-control labels \(P3400\)\.  Both must specify the same$/

## gcc/cp/parser.cc#8

The forward declarations: P3400's deferred label parse (GCC-960), then the
declarator-position work's.

- +/^static void cp_parser_replay_declaration_contracts$/

## gcc/cp/parser.cc#67

`cp_parser_late_contract_condition`: the deferred P3400 label (GCC-960),
P4451's lookup checks, then P4283's deferred requires-clause.

- +/^  \/\* P4451: a terminating assertion's lookup checks, for the same reason; a$/
- +/^  \/\* Replay a deferred requires-clause \(P4283\) first\./

## gcc/cp/parser.cc#78.4

The end of P3099's `cp_parser_contract_message`, then the comment of
P3400's `cp_parser_assertion_control_expression` (GCC-960).

- +/^\/\* Parse the constant-expression of an assertion-control-specifier, the next$/

## gcc/cp/parser.cc#78.9

P3400's `cp_parser_assertion_control_specifier`, then the comment of
P4451's `cp_parser_terminating_specifier`.

- +/^\/\* P4451: parse an assertion-control-specifier .< terminating >. while$/

## gcc/cp/parser.cc#79

`cp_parser_contract_assert`: P4451's specifier, then P3400's label and
P4283's requires-clause.

- +/^  \/\* Parse optional assertion-control-specifier: < constant-expression >  \*\/$/

## gcc/cp/parser.cc#84

`cp_parser_contract_assert`'s call to `grok_contract`, P4451's marking of a
terminating assertion, then the renamed postcondition flag.

- +/^  if \(terminating && contract && contract != error_mark_node\)$/ -/^  processing_postcondition = old_pc;$/
- +/^  processing_postcondition_predicate = old_pc;$/ -/^  processing_postcondition = old_pc;$/

## gcc/cp/parser.cc#92

`cp_parser_function_contract_specifier`: whether to defer, P4451's
specifier, then P3400's label and P4283's requires-clause.

- +/^  \/\* P4451: .< terminating >. without P3400\.  Its lookup checks run where a$/
- +/^  \/\* Parse optional assertion-control-specifier: < constant-expression >$/

## gcc/cp/parser.cc#100

The deferred path's call to `grok_contract`, P4451's marking of a
terminating assertion, then the result-name attributes and P3098 captures.

- +/^      if \(terminating && contract != error_mark_node\)$/
- +/^      \/\* Carry any attribute-specifier-seq written on the result name to the$/

## libstdc++-v3/testsuite/18_support/contracts/feature_test_macros.cc#0

The value check of `__cpp_lib_contracts` is P4451's, which raises it
under `-fcontracts-p3850`; the rest of the test is P4301's.

- +/^#elif __cpp_lib_contracts <= 202502L/
- +/^# error "__cpp_lib_contracts has the wrong value"$/

## gcc/c-family/c-opts.cc#4

`c_common_post_options`' contracts option rule and the warnings for the
options that have no effect (`6900-cross-paper-options`), then the P4405
proof optimization level and the strategy defaults it selects
(`4950-p4405-prove-optimize`).  Cut at the latter.

- +/^  \/\* The P4405 proof optimization level \(DECISIONS\.md E162-E168, E205\):$/

## gcc/c-family/c.opt#9.15

The record of `-fcontracts-prove-fold` (`4910-p4405-fold`), then the
comment heading the proof optimization level's enumeration
(`4950-p4405-prove-optimize`).  Cut at the comment.

- +/^; D4405: the proof optimization level\./

## gcc/doc/invoke.texi#11.75

The end of the item of `-fcontracts-prove-callee-premises`
(`4920-p4405-shadow`), then the item of `-fcontracts-prove-optimize=`
(`4950-p4405-prove-optimize`).  Cut at the latter.

- +/^@opindex fcontracts-prove-optimize$/
