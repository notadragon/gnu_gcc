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

## gcc/cp/contracts.cc#133.5
`resolve_contract_label` is a single 361-line contiguous insertion whose
interior belongs to several different commits.  Cut it after the
local_violation_label trampoline and again before the allowed_semantics
restriction, so the queryable_label facet and the semantic-restriction logic
can be claimed separately.

- +/^  \/\* Check for queryable_label facet/
- +/^  \/\* Compute the flag-independent label restriction/

## gcc/c-family/c.opt#0

One hunk declares three unrelated warnings in a row --
`-Wcontract-configuration` (P3595), `-Wcontract-constexpr-side-effect` (the
P2900 constant-evaluation side-effect warning) and
`-Wcontract-invalid-label-facet` (P3400).  Nothing but their alphabetical
neighbourhood puts them together.  Cutting on each option name lets the three
commits that introduce the warnings declare their own, so the
constexpr-side-effect commit does not have to be preceded by P3400 merely to
have an option to name.

- +/^Wcontract-constexpr-side-effect$/
- +/^Wcontract-invalid-label-facet$/

## gcc/cp/contracts.cc#25.0

This hunk replaces the body of the old `check_param_in_postcondition` with the
walk that `check_postcondition_param_odr_uses` performs (the postcondition
odr-use rework), and then, in the same run of added lines, opens the
documentation comment of `diagnose_coroutine_postcondition_params`, which
belongs to the coroutine fix.  Cut at the comment so each half travels with
the function it describes.

- +/^\/\* \[dcl\.fct\.def\.coroutine\]\/5 says a coroutine behaves as if/

## gcc/cp/contracts.cc#56

`start_function_contracts`'s `contract_any_active_p` early-out runs straight
into the block that records a function whose postconditions can throw as
having a cleanup that might throw, with no unchanged line between.  The
early-out is the dynamic-selection work's activity query; the recording is
what makes the single return-value cleanup possible.  Cut at the comment so
each half travels with the commit that needs it.

- +/^  \/\* A postcondition check runs after the returned object has been$/

## gcc/cp/contracts.cc#72.0

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

## gcc/cp/contracts.cc#204.34
The tail of `contract_local_handler_always_rethrows_p` runs straight into the
documentation comment of `emit_check_for_semantic` -- anchors.py cannot see
the boundary because the comment's continuation lines are indented text, not
`*`-prefixed, so it does not read as a comment head.  The comment describes
P3595's refactoring of the base emitter, not the rethrow analysis.  Cut at
the comment so the function and its documentation travel together.

- +/^\/\* Emit the check body for CONTRACT under a single, statically known$/

## gcc/cp/contracts.cc#211
One hunk removes the terminate-wrapper and violation-handler-wrapper
preamble that `2200-libcontracts` replaces with `__cxa_` entry points, and
adds two unrelated things: P3595's `unshare_expr` of the condition, needed
because the emitter is now called once per dynamic-dispatch arm, and the
`kind` local that `declare_cxa_entry_point` takes.  Cut before `kind`, and
send every removed line to piece 1 -- the removals are the libcontracts
rewrite, not the unshare.

- +/^  \/\* Determine the assertion kind for entry point selection\./ -/^  tree terminate_wrapper = terminate_fn;$/

## gcc/cp/contracts.cc#212
Two consecutive additions with nothing in common: `is_noexcept`, which
recognises P4298's two nonthrowing semantics, and the P3400
bypass-rethrowing-local-handler shortcut.  Cut between them.

- +/^  \/\* If the label's local violation handler answers an evaluation_exception by$/

## gcc/cp/contracts.cc#214
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
paper define its own macro.  The last pair is `-fcontracts-allow-assume`,
which belongs to `6000-p3100-core` along with the `assume` semantic itself.

- +/^      if \(flag_contracts_p3290\)$/
- +/^      if \(flag_contracts_p3400\)$/
- +/^      if \(flag_contracts_p3098\)$/
- +/^      if \(flag_contracts_p4283\)$/
- +/^      if \(flag_contracts_p3100\)$/
- +/^      if \(flag_contracts_p4298\)$/
- +/^      if \(flag_contracts_p4301\)$/
- +/^      if \(flag_contracts_allow_assume\)$/

## gcc/c-family/c.opt#2

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

## gcc/c-family/c.opt#3

The `-fcontract-evaluation-semantic=` option record and its help text.  The
record line is widened from `C++ ObjC++` to `C C++ ObjC ObjC++` by P4299;
the help line spells out all seven semantics, so it names P3100 (`assume`)
and P4298 (`noexcept_*`) in one string and cannot be divided.  Cut between
them; the two removed lines stay with piece 0, which is the commit that
rewrites the record.

- +/^-fcontract-evaluation-semantic=\[ignore\|observe\|enforce\|quick_enforce\|assume/

## gcc/cp/g++spec.cc#0

The driver's copy of the option list: one `case OPT_fcontracts_pNNNN:` label
per paper, then the four `-std=` labels at or above C++26, then the comment
explaining the whole block.  The label run is strictly line-per-paper, so
cut it that way.  The `-std=` labels and the comment are the reason the
block exists at all -- linking the contracts runtime -- and stay with
`2200-libcontracts`.

- +/^\tcase OPT_fcontracts_p3098:$/
- +/^\tcase OPT_fcontracts_p3099:$/
- +/^\tcase OPT_fcontracts_p3100:$/
- +/^\tcase OPT_fcontracts_p3290:$/
- +/^\tcase OPT_fcontracts_p3400:$/
- +/^\tcase OPT_fcontracts_p3850:$/
- +/^\tcase OPT_fcontracts_p4283:$/
- +/^\tcase OPT_fcontracts_p4298:$/
- +/^\tcase OPT_fcontracts_p4301:$/
- +/^\tcase OPT_std_c__26:$/

## gcc/doc/invoke.texi#1

The option-summary synopsis.  Its first two lines are P3595's configuration
options and its last two are P3100's sanitizer-routing options; the five
between them each pack two to four unrelated flags onto one line, which
`@gccoptlist` renders as a run-on list.  A line is the smallest thing a cut
can move, so those five are indivisible and travel together to the
cross-paper commit.

- +/^-fcontracts-allow-assume  -Wcontract-configuration$/
- +/^-fsanitize-semantic=@var\{check\}:@var\{semantic\}$/

## gcc/doc/invoke.texi#2

Three paragraphs added to the `-fcontract-evaluation-semantic=` semantic
list: `assume` (P3100, and its `-fcontracts-allow-assume` gate) and the two
P4298 nonthrowing semantics.  Cut between them.

- +/^'@code\{noexcept_enforce\}' As @code\{enforce\}/

## gcc/doc/invoke.texi#3.15

The paper roll-call: the tail of the `-fcontracts-p3850` description, then
ten `@opindex fcontracts-pNNNN` lines, then the ten `@item` / `@itemx` lines
they index.  Every one of those twenty is line-per-paper, so cut all of
them; each paper claims its own `@opindex` and its own `@item`.  Piece 0,
the `-fcontracts-p3850` description, is `2900-umbrella-option`.

- +/^@opindex fcontracts-p3097$/
- +/^@opindex fcontracts-p3098$/
- +/^@opindex fcontracts-p3099$/
- +/^@opindex fcontracts-p3100$/
- +/^@opindex fcontracts-p3290$/
- +/^@opindex fcontracts-p3400$/
- +/^@opindex fcontracts-p4283$/
- +/^@opindex fcontracts-p4298$/
- +/^@opindex fcontracts-p4299$/
- +/^@opindex fcontracts-p4301$/
- +/^@item -fcontracts-p3097$/
- +/^@itemx -fcontracts-p3098$/
- +/^@itemx -fcontracts-p3099$/
- +/^@itemx -fcontracts-p3100$/
- +/^@itemx -fcontracts-p3290$/
- +/^@itemx -fcontracts-p3400$/
- +/^@itemx -fcontracts-p4283$/
- +/^@itemx -fcontracts-p4298$/
- +/^@itemx -fcontracts-p4299$/
- +/^@itemx -fcontracts-p4301$/

## libstdc++-v3/include/std/contracts#12

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

## gcc/doc/invoke.texi#3.18

The last three lines of the prose that names every paper in one sentence,
running straight into the `@item` header of P3100's
`-fsanitize-semantic=`.  Cut at the header so the description that follows
it, which is already P3100's, is not separated from the item it describes.

- +/^@opindex fsanitize-semantic$/

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

- +/^  if \(const char\* __r = violation\.report\(\); __r && __r\[0\] != '\\0'\)$/
- +/^  std::cerr << "contract violation in function "$/
- +/^    case std::contracts::evaluation_semantic::noexcept_observe:$/
- +/^    default:$/#1

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

## gcc/cp/constexpr.cc#19
The contract case's speculative-evaluation block, written by two commits
whose lines interleave.  `1020-constexpr-side-effect` introduces the block --
the `eval`/`modifies_outside`/`modified_obj` locals, the tracked trial, the
re-run and its warning -- and `1025-contract-discardable-evaluation` reaches
into it three times: to declare the `contract_report_tracker` before the
trial (it has to span the re-run decision, so it cannot live in the trial's
scope), to dismiss it when there is no re-run, and to roll it back when there
is.  Six cuts, seven pieces, alternating 1020 / 1025.

The regexes distinguish the two `if (ctrct_non_const_p && modifies_outside)`
lines by the negation, and the two `constexpr_ctx new_ctx = *ctx;` lines by
their indentation; both pairs would otherwise match twice and be rejected.
No `-/regex/` is given, so the removed upstream lines stay with piece 0.

- +/^\t\/\* Spans the re-run decision below, not just the trial/
- +/^\t{$/
- +/^\tif \(!\(ctrct_non_const_p && modifies_outside\)\)$/
- +/^\tif \(ctrct_non_const_p && modifies_outside\)$/
- +/^\t    \/\* Roll the trial's records back before the real evaluation$/
- +/^\t    constexpr_ctx new_ctx = \*ctx;$/

## gcc/cp/contracts.cc#104
`update_contract_arguments`' comment ends with P3595's note that the
non-deferred path copies and remaps more than it needs, and runs straight on
into the warning that the deferred path's copy must stay unconditional --
there is no unchanged line between them.  The first belongs to the
configuration work that wrote the remap; the second to the declarator-position
change, which is what made every contract deferred and so made that copy
load-bearing.  Cut between them.

- +/^    The copy is unconditional on purpose, and it is tempting to think it$/
