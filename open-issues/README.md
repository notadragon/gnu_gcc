# Open Issues on This Branch

What is currently broken on the `contracts-p3850` branch of GCC -- whatever
its origin.  If you hit something while using this compiler, look here first:
a row means we already know, and tells you whether there is a way around it.

A row is removed, and any file it points to in this directory deleted, as
soon as the issue is **fixed on this branch**.  Git history is the record;
nothing is archived in place.

**This file is the only ledger of owed work on this branch.**  Every known
defect and deferral is a row in the first table, and every owed
cleanup (style, comments, dead code, duplication) or task (a missing test, an
owed run, a blocked chore) is a row in the task table.  Review notes, plans and
research trails written elsewhere are records of reasoning, not lists of work:
an item found there that is not a row here has been lost.  Record it here
first, then write anywhere else.  Divergences are recorded in their own
table, [Divergences](#divergences), and are not owed work.

This is a different question from the one
[`../bug-reports/`](../bug-reports/README.md) answers.  That directory tracks
bugs that reproduce on **stock upstream** GCC, including ones already fixed
here, and a row there survives until *upstream* fixes it.  An upstream bug
not yet fixed here appears in both; its single writeup lives in
`bug-reports/` and the row below links to it.

## What the columns mean

**Kind** is one of `defect` (wrong, and we intend to fix it) or `deferred`
(known, currently out of scope) in the first table, `divergence` (our
compilers disagree and the standard does not clearly settle which is right)
in the [Divergences](#divergences) table, and
`cleanup` (no behaviour change: style, comments, naming, dead code,
duplication) or `task` (owed work that is neither a defect nor a cleanup) in
the task table; `cleanup` rows (no behaviour change: style, comments, naming,
dead code, duplication) live in [`CLEANUP.md`](CLEANUP.md).  A new issue is always recorded as `defect` when it is discovered;
it becomes `deferred` only when the user has said so.

A `cleanup` or `task` row needs no writeup, reproducer or watch test: its
**Where** column names the file and symbol, and one line of summary is enough.
A defect row always has a writeup, and a watch test in both compilers' suites
(`g++.dg/contracts/cpp26/open-bug-*.C` here), xfailed per measured behaviour.

**Upstream** distinguishes three states.  A link means a PR exists.
`None found (searched <date>)` means Bugzilla was searched and nothing
matched.  `UNKNOWN` means nobody has looked yet.  `--` means the issue is
ours alone with nothing upstream to link -- either because the trigger needs
one of this fork's own extensions, which are not upstream, or because a
regression lives in machinery shared with upstream but does not reproduce
there.  A trailing `*` marks a PR filed from this work.

**Details** links the writeup.  Most rows point into `bug-reports/`, because
most branch-only issues are also upstream bugs not yet fixed here; an issue
gets its own `gcc-NN/` directory here only when it does not reproduce
upstream at all.

IDs come from one sequence shared with `bug-reports/` and with the cleanup and
task table, and are never reused.  Allocate from the **Next ID** line below
and increment it in the same commit: both tables delete rows, so the highest
ID visible is not a reliable counter.

## The table

**Next ID:** GCC-709

| ID | Symptom | Kind | Upstream | Details |
|----|---------|------|----------|---------|
| GCC-2 | Constant evaluation accepts forming the address of a member, or of a virtual base, before its constructor begins | deferred | [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (partial) | [../bug-reports/gcc-02/gcc-02-constexpr-vbase-before-ctor.md](../bug-reports/gcc-02/gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-17 | `this` is accepted in the trailing return type of an explicit-object member function, where it is ill-formed | deferred | [PR127290](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127290) * | [../bug-reports/gcc-17/gcc-17-this-in-xobj-declaration.md](../bug-reports/gcc-17/gcc-17-this-in-xobj-declaration.md) |
| GCC-27 | A friend declaration that redeclares an ordinary namespace-scope function with a different or an added contract is diagnosed only when a later declaration happens to flush the redeclaration-match queue (measured 2026-10-07 at `747de38a5a1`): nothing at the end of the befriending class or of the translation unit flushes it, so the reproducer, with no contract-bearing declaration after the class, is accepted.  When a later declaration does flush it, the friend's predicate is parsed outside the befriending class (a member of it named there is "not declared") and the diagnostic carries that declaration's "In function" context.  Two friend declarations of one function are diagnosed (`contract-friend-deferred-mismatch.C`) | deferred | [PR127291](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127291) * | [../bug-reports/gcc-27/gcc-27-deferred-friend-contract-mismatch.md](../bug-reports/gcc-27/gcc-27-deferred-friend-contract-mismatch.md) |
| GCC-39 | Explicit instantiation of a variadic template rejects a cv-qualified pack argument, so the instantiation is not emitted (not a contracts bug) | deferred | [PR126797](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126797) | [../bug-reports/gcc-39/gcc-39-explicit-inst-pack-cv.md](../bug-reports/gcc-39/gcc-39-explicit-inst-pack-cv.md) |
| GCC-103 | `__tu_quick_enforce_wrapper` is an out-of-line `noipa` call not marked `noreturn`, costing a frame and spills on every cold path. Deferred by the user 2026-08-24: fixing it properly needs other IPA-related GCC defects fixed first; a dedicated piece of work, started only when the user asks.  Includes finding a more targeted fix than the blanket noipa trap wrapper, so assertions optimize better (was GCC-117) | deferred | -- | `gcc/cp/contracts.cc`: `declare_quick_enforce_wrapper` |
| GCC-104 | Every `-std=c++26` link pulls `-lstdc++exp -lcontracts` and two `-u` marker symbols unconditionally (LSTD-4). User disposition: QoI, not fixing now; a marker plus `--as-needed` is the preferred eventual shape | deferred | -- | `gcc/cp/g++spec.cc` |
| GCC-105 | C: a contract on a non-defining prototype has no effect on the definition, where Clang applies it (P2900 applies contracts across redeclarations). Deferred by the user 2026-08-07, both the implementation and the D4299 paper rule | deferred | -- | `gcc/c/c-parser.cc` (`pending_contracts`), `gcc/c/c-decl.cc` (`duplicate_decls`) |
| GCC-106 | Extension: an additional caller-side check when calling through a virtual function's interface. Deferred by the paper author as a possible future extension | deferred | -- | -- |
| GCC-109 | Route LeakSanitizer reports through the contract-violation handler (assume/enforce), as ASan/UBSan/TSan/MSan are.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-110 | Sweep the remaining `-fsanitize=` checks (beyond ASan/UBSan/TSan/MSan) for contract routing.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-113 | A dynamic/label-selected semantic that can only yield nonthrowing results should make the contract not potentially-throwing.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `gcc/cp/contracts.cc:422-447` |
| GCC-115 | Investigate integrating libstdc++'s `__glibcxx_requires_*`/`__glibcxx_check_*`/debug-mode assertion macros with `contract_assert`.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-116 | Explore adding `pre` macros (pre/body idiom, as in BDE) to libc++/libstdc++ headers.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-119 | P3595: add function-level and module-name match criteria to configuration entries.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `contracts-config.cc:357`, `contracts-config.cc:593-632` |
| GCC-121 | The remaining P3100 UB categories (Group C whole-program cases, VLA/flexible-array/C cases, and the ~35 with no locally checkable site); int->float, `[[noreturn]]` returns and pointer bounds are GCC-124, GCC-122 and GCC-125 (was also GCC-112).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-122 | `[[noreturn]]` function returns (`dcl.attr.noreturn.eventually.returns`) check.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-123 | Other null-pointer cases (`conv.ptr.virtual.base`, `expr.dynamic.cast.pointer.lifetime`, `expr.static.cast.downcast.wrong.derived.type`, `expr.mptr.oper.member.func.null`).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `expr.dynamic.c`, `expr.static.c` |
| GCC-124 | Float conversion checks F.3.6 `conv.double.out.of.range`, F.3.8 `conv.fpint.int.not.represented`, F.3.17 `expr.static.cast.fp.outside.range`, including the open question of how to separate F.3.6 from F.3.17.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `conv.fpint.in`, `expr.static.c` |
| GCC-125 | P3100: implement a check for pointer-arithmetic bounds (`expr.add.sub.diff.pointers`).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-129 | Consolidate consecutive location/namespace config entries into a map instead of a linear scan (performance, low priority).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `ContractOptions.cpp:737,765`, `contracts.cc:877,1189` |
| GCC-131 | Measure, and maybe improve, a plain-semantic check inside a `noexcept` function using the ABI's localized terminate instead of unwind termination.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-140 | P3595 per-module configuration stored in the CMI (callee uses its module's configuration, caller the TU's).  Deferred by the user 2026-09-25: revisit when other work runs out; do not raise it before then | deferred | -- | -- |
| GCC-151 | Explore supporting the `assume` semantic in more places (an optimiser assumption where sound); it lowers to `ignore` for now, which is valid and sometimes best.  Deferred by the user 2026-09-25.  Includes real codegen for `assume` on explicit contracts (was GCC-120) | deferred | -- | `gcc/cp/contracts.cc` (`emit_check_for_semantic`) |
| GCC-154 | Measure the cost of passing by-value parameters by reference in outlined checks, and re-run the `-fcontract-disable-optimized-checks`/P3097 suites.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| GCC-695 | P3400R4's dimension layer and `hardened` are not in `<contracts>`: `dimensioned_label`, `dimension_list`, `is_dimensioned_label`, the label combination operator's rejection of overlapping dimensions, and the labels `opt`, `audit` and `hardened` (whose `compute_semantic` is implementation-defined).  The rest of the P3400R4 library surface is implemented (GCC-595, M30-GCC).  Deferred by the user 2026-10-10 (DECISIONS.md E59 (b)) | deferred | -- | `libstdc++-v3/include/std/contracts` |
| GCC-696 | The rethrow-bypass analysis bails on a local handler defined out of line later in the translation unit (its body does not exist when the check is built); it would need the decision deferred to the end of the TU.  `p3400-bypass-rethrowing-local-handler-negative-codegen.C` case 5 pins the bail.  Deferred by the user 2026-10-10 (DECISIONS.md E62 (b): no delayed evaluation) | deferred | -- | `gcc/cp/contracts.cc` (`contract_local_handler_always_rethrows_p`) |
| GCC-617 | C -- a statement expression in a prototype's contract is rejected | deferred | -- | [gcc-617-c-stmt-expr-in-prototype.md](gcc-617/gcc-617-c-stmt-expr-in-prototype.md) |
| GCC-635 | `g++.dg/tree-ssa/ssa-dse-1.C` fails one `scan-tree-dump-times` row at base `8802cd9fdbe`; stock trunk at the same commit fails it identically (not a contracts bug; upstream's stale expectation, likely after `a382bed036a`).  Deferred by the user 2026-10-06: expected to clear on the next rebase | deferred | None found (searched 2026-10-06) | [gcc-635/gcc-635-ssa-dse-1-at-base.md](gcc-635/gcc-635-ssa-dse-1-at-base.md) |
| GCC-644 | A local named in `typeid` of a polymorphic glvalue in a lambda with no capture-default is not captured and not diagnosed (stock trunk too, a GCC 17 regression from r17-2200; not a contracts bug).  Deferred by the user 2026-10-06 | deferred | None found (searched 2026-10-06) | [../bug-reports/gcc-644/gcc-644-typeid-polymorphic-capture.md](../bug-reports/gcc-644/gcc-644-typeid-polymorphic-capture.md) |
| GCC-660 | A variable declared in a GNU statement expression in the postcondition of a generic lambda whose return type is deduced ICEs in `gimplify_var_or_parm_decl` when the operator() is instantiated (a trailing return type compiles).  Deferred by the user 2026-10-06: statement expressions are a GNU extension; fix only if the fix turns out trivial | deferred | -- | [gcc-660-stmt-expr-local-generic-lambda-deduced-post.md](gcc-660/gcc-660-stmt-expr-local-generic-lambda-deduced-post.md) |
| GCC-661 | A GNU statement expression in the contract of an ordinary function declaration is rejected ("statement-expressions are not allowed outside functions"), e.g. `int f (int x) pre (({ x > 0; }));`; noticed during M5, not investigated.  Deferred by the user 2026-10-06: statement expressions are a GCC extension, not worth chasing | deferred | -- | -- |
| GCC-683 | `noexcept(delete (D *) 0)` is true when `D`'s destructor is potentially-throwing: `delete_sanity` turns the delete of a null pointer constant into a no-op before the destructor call is built (stock too; not a contracts bug).  Watch test `open-bug-noexcept-delete-null.C`.  Deferred by the user 2026-10-09 (DECISIONS.md E50 (c)): not a contracts bug; upstream PR65174 | deferred | [PR65174](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65174) | [../bug-reports/gcc-683/gcc-683-noexcept-delete-null.md](../bug-reports/gcc-683/gcc-683-noexcept-delete-null.md) |

## Divergences

Where our compilers (GCC, Clang, and the EDG front end) behave differently
and the standard does not clearly settle which is right.  The divergence is
accepted: a row records it so that nobody rediscovers it, and is not owed
work.  A row is deleted if the compilers come to agree.  Same ID sequence as
the table above.

| ID | Symptom | Kind | Upstream | Details |
|----|---------|------|----------|---------|
| GCC-702 | A precompiled header made under a different contract configuration (`-fcontract-evaluation-semantic=`, or a configuration file whose parsed entries differ, including the same file with changed contents) than the translation unit's: GCC records the configuration in the PCH and rejects it (`-Winvalid-pch` says "contract configuration does not match"; without the header source that is an error), Clang records none and uses the PCH with the TU's configuration (CLANG-683), and the EDG front end does not use it (the options and the configuration file's text are PCH events, so automatic PCH mode recreates it, also for `contract-config-rewritten.C`'s rewrite with other text but the same entries, which GCC uses: measured by hand in M38-EDG) (EDG-129).  In none of the three does the header's configuration override the TU's; PCHs are outside the standard.  Recorded 2026-10-11 (M37-GCC) | divergence | -- | `g++.dg/pch/contract-{semantic-mismatch,config-mismatch,config-match,config-changed,config-rewritten}.C`; Clang `pch-contract-semantic.cpp`, `pch-contract-config-file-changed.cpp`; EDG `config/pch_file` |
| GCC-703 | A contract assertion repeated on a later declaration (the definition) and violated: GCC reports the definition's assertion, in constant evaluation and in `contract_violation::location()`, for every kind of function.  Clang does too, except for a member of a class template whose return type is not deduced, where it reports the in-class declaration's (llvm CLANG-684); the EDG front end reports the first declaration's, except that through edg-gxx a template is put out as written and g++ reports the definition's (edgcpp EDG-130, where the row is written up).  The location is implementation-defined and the assertions are equivalent.  Recorded 2026-10-11 (M38-EDG) | divergence | -- | `contract-repeated-location.C` (`location()`), `contract-repeated-location-constexpr.C` and `contract-classtemplate-pack-member-fold-constexpr.C` (constant evaluation); EDG `codegen/repeated_contract_location` |
| GCC-708 | The comment of a contract whose predicate involves macros.  GCC (GCC-707's fix): the predicate's source text where the contract was written, macro invocations included whole (`NEG (x)`, `X < ZERO`); for a contract written by a macro, the text of the replacement list or of the argument (`MY_PRE (x < 0)` gives `x < 0`); otherwise the predicate's tokens.  Clang: always the source text of the predicate's expansion range, so a wrapper macro gives its invocation (`MY_PRE (x < 0)`, `PRE_NEG`), and the same as GCC otherwise.  The EDG front end: the predicate's tokens after expansion (`( ( x ) < 0 )`), and through edg-gxx g++'s printed expression.  The comment is implementation-defined.  Recorded 2026-10-12 | divergence | -- | `g++.dg/contracts/cpp26/contract-comment-macro.C`, `contract-comment-macro-untracked.C`, `gcc.dg/contracts/c-contract-comment-macro.c`; Clang CLANG-687, EDG EDG-133 |

## Tasks

Owed work that is not a defect and not a cleanup: a missing test, an owed
run, a decision owed, unimplemented scope.  Same ID sequence as the table
above.  Cleanups live in [`CLEANUP.md`](CLEANUP.md).

| ID | Summary | Kind | Where |
|----|---------|------|-------|
