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

## gcc/cp/contracts.cc#156.12
`resolve_contract_label` is a single 361-line contiguous insertion whose
interior belongs to several different commits.  Cut it after the
local_violation_label trampoline and again before the allowed_semantics
restriction, so the queryable_label facet and the semantic-restriction logic
can be claimed separately.

- +/^  \/\* Check for queryable_label facet/
- +/^  \/\* Compute the flag-independent label restriction/

## gcc/c-family/c.opt#0

One hunk declares two unrelated warnings in a row --
`-Wcontract-configuration` (P3595) and `-Wcontract-invalid-label-facet`
(P3400).  Nothing but their alphabetical neighbourhood puts them together.
Cut on the second option name so each commit declares its own.

- +/^Wcontract-invalid-label-facet$/

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

## gcc/cp/contracts.cc#85.0

A single 105-line insertion that rewrites the body of `apply_postconditions`
and then, with no unchanged line between, opens the documentation comment of
`wrap_postconditions_in_retval_cleanup`.  The rewrite belongs to the
result-binding work; the cleanup and its comment belong to the
throwing-postcondition work.  Cut at the comment.  The body itself is not cut
further: its retval stand-in prologue, the P3098 capture-flag gating loop and
the copy-back epilogue are interleaved, and the closing brace of the function
sits at the end of the epilogue, so any interior cut leaves one piece with an
unbalanced brace.

- +/^\/\* Wrap STMTS -- the postcondition checks of FNDECL/

## gcc/cp/contracts.cc#229.12
The tail of `contract_local_handler_always_rethrows_p` runs straight into the
documentation comment of `emit_check_for_semantic` -- anchors.py cannot see
the boundary because the comment's continuation lines are indented text, not
`*`-prefixed, so it does not read as a comment head.  The comment describes
P3595's refactoring of the base emitter, not the rethrow analysis.  Cut at
the comment so the function and its documentation travel together.

- +/^\/\* Emit the check body for CONTRACT under a single, statically known$/

## gcc/cp/contracts.cc#236
One hunk removes the terminate-wrapper and violation-handler-wrapper
preamble that `2200-libcontracts` replaces with `__cxa_` entry points, and
adds two unrelated things: P3595's `unshare_expr` of the condition, needed
because the emitter is now called once per dynamic-dispatch arm, and the
`kind` local that `declare_cxa_entry_point` takes.  Cut before `kind`, and
send every removed line to piece 1 -- the removals are the libcontracts
rewrite, not the unshare.

- +/^  \/\* Determine the assertion kind for entry point selection\./ -/^  tree terminate_wrapper = terminate_fn;$/

## gcc/cp/contracts.cc#239
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

The per-paper feature-test macros: nine `if (flag_contracts_pNNNN)` /
`cpp_define (...)` pairs in one contiguous insertion, with no unchanged line
between them.  Strictly line-per-paper, so cut on each `if` and let every
paper define its own macro.  P4299's comes after P4301's, with a comment of
its own, so cut on the comment.  The last pair is `-fcontracts-allow-assume`,
which belongs to `6000-p3100-core` along with the `assume` semantic itself.

- +/^      if \(flag_contracts && flag_contracts_p3290\)$/
- +/^      if \(flag_contracts && flag_contracts_p3400\)$/
- +/^      if \(flag_contracts && flag_contracts_p3098\)$/
- +/^      if \(flag_contracts && flag_contracts_p4283\)$/
- +/^      \/\* P3100, like P3290, specifies no language feature-test macro/
- +/^      if \(flag_contracts && flag_contracts_p4298\)$/
- +/^      if \(flag_contracts && flag_contracts_p4301\)$/
- +/^      \/\* P4299's keywords, _Pre, _Post and _ContractAssert/
- +/^      if \(flag_contracts && flag_contracts_allow_assume\)$/

## gcc/c-family/c-cppbuiltin.cc#2

The C side of the same macros: P4299's own vendor macro, then
`-fcontracts-allow-assume`'s, which is `6000-p3100-core`'s.  Cut at the
latter's comment.

- +/^  \/\* P4299 C contracts honour -fcontracts-allow-assume too\.  \*\/$/

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

## gcc/c-family/c.opt#5

The `-fcontract-*` records that sort between `-fcontract-disable-optimized-checks`
and `-fcontracts-client-check=`, in collating order: P3400's
`-fcontract-bypass-rethrowing-local-handler`, the base facility's
`-fcontract-comment-from-expression`, P3595's configuration and group
options, and P3100's `-fcontracts-allow-assume`.  Cut before each commit's
first record.

- +/^fcontract-comment-from-expression$/
- +/^fcontract-configuration=$/
- +/^fcontracts-allow-assume$/

## gcc/cp/g++spec.cc#3

The driver's copy of the option list: one `case OPT_fcontracts_pNNNN:` label
per paper, then the comment explaining the whole block.  The label run is
strictly line-per-paper, so cut it that way.  The comment is the reason the
block exists at all -- linking the contracts runtime -- and stays with
`2200-libcontracts`, as do the lines recording the value of `-fcontracts`
(before the first paper label) and of the last per-paper flag (after the
last one).

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
- +/^\t  \/\* As LangEnabledBy in c\.opt/

## gcc/c-family/c.opt.urls#0

The URLs of the two warnings that `c.opt#0` also declares in a row: P3595's
`-Wcontract-configuration`, then P3400's `-Wcontract-invalid-label-facet`.
Cut on the second option name, as there.

- +/^Wcontract-invalid-label-facet$/

## gcc/c-family/c.opt.urls#1

The URLs of the options after `-fcontract-disable-optimized-checks`:
P3400's `-fcontract-bypass-rethrowing-local-handler`, the base facility's
`-fcontract-comment-from-expression`, P3595's three configuration options,
and P3100's `-fcontracts-allow-assume`.  Cut at each owner's first option.

- +/^fcontract-comment-from-expression$/
- +/^fcontract-configuration=$/
- +/^fcontracts-allow-assume$/

## gcc/c-family/c.opt.urls#2

The URLs of the eleven `-fcontracts-pNNNN` options, one per paper.  Cut at
each.

- +/^fcontracts-p3098$/
- +/^fcontracts-p3099$/
- +/^fcontracts-p3100$/
- +/^fcontracts-p3290$/
- +/^fcontracts-p3400$/
- +/^fcontracts-p3850$/
- +/^fcontracts-p4283$/
- +/^fcontracts-p4298$/
- +/^fcontracts-p4299$/
- +/^fcontracts-p4301$/

## gcc/doc/invoke.texi#2

The option-summary synopsis lines after `-fcontract-disable-optimized-checks`:
P3400's `-fno-contract-bypass-rethrowing-local-handler`, then the base
facility's `-fcontract-comment-from-expression`.  Cut between them.

- +/^-fcontract-comment-from-expression$/

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
P3100's `-fcontracts-p3100` is accepted for C too.  Cut before that
paragraph.

- +/^@option\{-fcontracts-p3100\} is accepted for C as well, with or without$/

## gcc/doc/invoke.texi#8

Paragraphs added to the `-fcontract-evaluation-semantic=` semantic list:
`assume` (P3100, and its `-fcontracts-allow-assume` gate), the two P4298
nonthrowing semantics, and how an assertion-control label (P3400) limits the
semantic.  Cut between them.

- +/^'@code\{noexcept_enforce\}' As @code\{enforce\}/
- +/^The semantic selected for a contract assertion, by this option or by the$/

## gcc/doc/invoke.texi#9

The descriptions after `-fcontract-disable-optimized-checks`: P3400's
`-fno-contract-bypass-rethrowing-local-handler`, then the base facility's
`-fcontract-comment-from-expression`.  Cut at the latter's first `@opindex`.

- +/^@opindex fcontract-comment-from-expression$/

## gcc/doc/invoke.texi#10.3

P3100's paragraph on the `ub:` groups of implicit contract assertions, then
P3595's `-fcontract-configuration` item headers.  Cut at the headers.

- +/^@opindex fcontract-configuration$/

## gcc/doc/invoke.texi#10.10

The end of P3100's `-fcontracts-allow-assume` description, then the item
header of `-fcontracts-p3850`.  Cut at the header.

- +/^@opindex fcontracts-p3850$/

## gcc/doc/invoke.texi#10.12

The paper roll-call: the tail of the `-fcontracts-p3850` description, then
nine `@opindex fcontracts-pNNNN` / `@opindex fno-contracts-pNNNN` pairs,
then the nine `@item` / `@itemx` lines they index.  Every one is per paper,
so cut at each; each paper claims its own pair and its own `@item`.  Piece 0,
the `-fcontracts-p3850` description, is `2900-umbrella-option`.

- +/^@opindex fcontracts-p3097$/
- +/^@opindex fcontracts-p3098$/
- +/^@opindex fcontracts-p3099$/
- +/^@opindex fcontracts-p3100$/
- +/^@opindex fcontracts-p3290$/
- +/^@opindex fcontracts-p3400$/
- +/^@opindex fcontracts-p4283$/
- +/^@opindex fcontracts-p4298$/
- +/^@opindex fcontracts-p4301$/
- +/^@item -fcontracts-p3097$/
- +/^@itemx -fcontracts-p3098$/
- +/^@itemx -fcontracts-p3099$/
- +/^@itemx -fcontracts-p3100$/
- +/^@itemx -fcontracts-p3290$/
- +/^@itemx -fcontracts-p3400$/
- +/^@itemx -fcontracts-p4283$/
- +/^@itemx -fcontracts-p4298$/
- +/^@itemx -fcontracts-p4301$/

## gcc/doc/invoke.texi#11

The two warnings in Warning Options: P3595's `-Wcontract-configuration`,
whose second paragraph is P4298's (the noexcept semantics it reports), then
P3400's `-Wcontract-invalid-label-facet`.  Cut at each.

- +/^The warning also reports an entry asking for the @code\{noexcept_enforce\}$/
- +/^@opindex Wcontract-invalid-label-facet$/

## libstdc++-v3/include/std/contracts#0

The `__glibcxx_want_*` requests for the contracts feature-test macros this
branch adds, in `version.def` order: P3290's four (`message`, `api`,
`implicit`, `labels` arrive with it), then P4301's `report`.  Cut before
`report`.

- +/^#define __glibcxx_want_contracts_report$/

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
- +/^  std::cerr << "contract violation in function "$/
- +/^    case std::contracts::evaluation_semantic::noexcept_observe:$/
- +/^    default:$/#2

The handler's semantic switch adds one `case`/print/`break` triple per
evaluation semantic, and two of them -- `noexcept_observe` and
`noexcept_enforce` -- are P4298's.  They are contiguous and each triple is
self-contained, so the third and fourth cuts hand exactly those six lines to
`4800-p4298`.  Dropping them leaves valid code with correct behaviour: the
`default:` arm already prints `unknown(N)` for a semantic it does not name,
which is what a history that has not yet added those semantics should say.

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

## gcc/cp/contracts.cc#1

The includes P3100's sanitizer routing needs (`varasm.h`, `ubsan.h`) run on
into `predict.h`, which the branch hint on the violation arm of a check
needs.  Cut before `predict.h`.

- +/^#include "predict\.h"$/

## gcc/cp/contracts.cc#122
`update_contract_arguments`' comment ends with P3595's note that the
non-deferred path copies and remaps more than it needs, and runs straight on
into the warning that the deferred path's copy must stay unconditional --
there is no unchanged line between them.  The first belongs to the
configuration work that wrote the remap; the second to the declarator-position
change, which is what made every contract deferred and so made that copy
load-bearing.  Cut between them.

- +/^    The copy is unconditional on purpose, and it is tempting to think it$/

## gcc/cp/contracts.cc#11.2

`dynamic_dispatch_semantics` (P3595 dynamic dispatch) lists the four C++26
semantics, and the P4298 noexcept pair under `-fcontracts-p4298`; cut the
P4298 block out so it can go with P4298.

- +/^  if \(flag_contracts_p4298\)$/
- +/^  return v;$/

## gcc/cp/contracts.cc#247.35

`build_contract_check` (P3595 dynamic dispatch) hands a dynamically selected
postcondition with captures to `build_dynamic_capture_selection` (P3098)
before building the dispatch; cut that hook out so it can go with P3098.

- +/^  \/\* A postcondition with captures chooses its semantic once, where the$/
- +/^  \/\* P3595 dynamic selection\.  Dispatch on the selector's runtime return value,$/

## gcc/cp/contracts.cc#21.4

The declaration of `contract_decl_map`, rewritten to hash its keys in a way a
precompiled header preserves, runs on into the documentation comment of
`contract_this_is_const_p`, which belongs to the `this`-is-const work.  Cut at
the comment.

- +/^\/\* True if a use of .this. at the current point is in a contract predicate,$/

## gcc/cp/parser.cc#61

P4299's `_Pre` and `_Post` keywords, inserted at the top of the P4283
rewrite of `cp_function_contract_specifier_intro`.  Cut between the comment
and the code; both pieces are P4299's.

- +/^  if \(token->keyword == RID_C_PRE\)$/

## gcc/cp/parser.cc#63.2

`cp_nth_token_is_contract_intro_p` is the P4283 lookahead; the two lines
that accept P4299's `_Pre` and `_Post` keywords in it are P4299's.

- +/^  if \(tok->keyword == RID_C_PRE \|\| tok->keyword == RID_C_POST\)$/
- +/^  return \(tok->type == CPP_NAME$/

## gcc/cp/contracts.h#14

The postcondition declarations, then P3098's capture helpers, then the
declaration of `member_template_spec_contracts` (explicit
specializations).

- +/^extern bool postcondition_capture_init_ok_p/
- +/^extern void member_template_spec_contracts/


## gcc/cp/pt.cc#7
`tsubst_contract` applies [dcl.contract.func]/7 to the substituted
postcondition (the odr-use rework) and, in the same run of added lines,
judges the declarations merged away against what that walk found (the
const rule across redeclarations).  Cut before the second.

- +/^  \/\* That walk can be the first to find a parameter odr-used, so judge the$/

## gcc/cp/contracts.cc#165

`grok_contract`'s comment, now built from the text the parser formed for
it, ahead of the P3400 compute_comment facet.  Cut at the blank line.

- +/^$/

## gcc/cp/contracts.cc#168

The same in `update_late_contract`, ahead of the facet.  Cut at the blank
line.

- +/^$/

## gcc/cp/parser.cc#48

In `cp_parser_late_contract_condition`, the comment formed from the
predicate's tokens, ahead of P3099's message.  Cut at the blank line.

- +/^$/#1

## gcc/cp/parser.cc#58

In `cp_parser_contract_assert`, P3098's rename of the postcondition flag,
then the first token of the predicate, kept for its comment.

- +/^  cp_token \*first = cp_lexer_peek_token \(parser->lexer\);$/

## gcc/cp/parser.cc#59

The comment of a `contract_assert`, ahead of P3099's message.  Cut at the
blank line.

- +/^$/#1

## gcc/cp/parser.cc#82

In `cp_parser_function_contract_specifier`, the comment of a contract
parsed in place, then P3099's message and the extension fields' call to
`grok_contract`, which replaces upstream's.

- +/^      tree message = cp_parser_contract_message \(parser\);$/ -/^      \/\* Build the contract\.  \*\/$/
