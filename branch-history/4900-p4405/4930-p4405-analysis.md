---
id: 4930-p4405-analysis
subject: 'c++, contracts: D4405: the analysis evaluation'
depends: [4920-p4405-shadow]
regenerates: []
fixes: []
---

## Rationale

Give each contract assertion an analysis evaluation, as D4405R0 now
proposes: a separate evaluation, the only one `prove` and `diagnose` ever
apply to, whose semantic -- `ignore` (the default, no analysis),
`diagnose` or `prove` -- is selected independently of the semantics of the
assertion's other, runtime, evaluations.

Configuration.  A P3595 entry whose `match` has `"analysis": true` selects
the analysis evaluation's semantic, in a lookup of its own through the same
list: only such entries match it, and every other lookup skips them
(`"analysis": false` is the same as no key).  With `"constexpr": true` such
an entry never matches; with a `caller` key, or with an output that has a
`dynamic` selector or a semantic other than `ignore`, `diagnose` and
`prove`, it is a configuration error, and is skipped.  Constant evaluation
is never analyzed, so an entry with `"constexpr": true` whose semantic is
`prove` or `diagnose` is a configuration error too.
`-fcontract-analysis-semantic=<s>` stands for such an entry after those of
the configuration sources, and without `-fcontracts-p4405` it gets the
P4405 no-flag warning and resolves as `ignore`.  The C front end shares the
configuration, so C assertions get all of it.

Resolution (D4405R0, "Adjusting to the Allowed Semantics").  The allowed
set of an assertion splits into its runtime part R and its analysis part
N, `prove` and `diagnose` (`contract_runtime_allowed_mask`,
`contract_analysis_allowed_mask`): the analysis evaluation may be `ignore`
only when R is not empty; a callee-side runtime evaluation is limited to
R, or to `ignore` when R is empty -- so
only a label, never the configuration, lets a callee-side check be
skipped; a caller-side one may always be `ignore`; with R and N both empty
there is no valid semantic.  `contract_config_resolve` then (1) looks up
both evaluations, (2) moves a configured runtime `prove` or `diagnose` into
the analysis evaluation, which takes the stronger (`ignore` < `diagnose` <
`prove`), leaving the runtime evaluation `ignore`, (3) strengthens the
analysis evaluation with the label's `analysis_semantic` facet, (4) adjusts
it to its allowed set (`prove`: `diagnose`, `ignore`; `diagnose`:
`ignore`, `prove`; `ignore`: `diagnose`, `prove`), and (5) adjusts the
runtime semantic to its set, which never holds `prove` or `diagnose`.  The
C++ front end adds step (6), `compute_semantic`: a `prove` or `diagnose`
result moves to the analysis evaluation, which must allow the stronger,
and the runtime evaluation becomes `ignore`, which must be in its own set;
any other result must be in the runtime set (`apply_compute_semantic_split`,
`p4405-analysis-compute.C`).  The analysis evaluation is resolved for the
callee-side query only; a caller-side query's analysis is just what its own
step 2 moves in (a configured caller-side `prove` is diagnosed at the call,
as before), and in constant evaluation there is none: step 2 does not
apply there, a constant evaluation is adjusted to the whole allowed set,
and a `prove` or `diagnose` it reaches, however (an entry without the
`constexpr` key, a flag, a label's set or `compute_semantic`), is
evaluated as `enforce` or `observe` (`contract_constant_evaluation_semantic`).
A dynamic selector's
value is adjusted the same way, as a runtime evaluation's
(`transform_semantic`).

Each contract assertion carries both semantics:
`CONTRACT_EVALUATION_SEMANTIC` holds the runtime semantic in its low byte
and the analysis semantic above it (`ensure_analysis_semantic`);
`ensure_evaluation_semantic` gives the runtime one, which is never `prove`
or `diagnose`.  The default semantic of a dynamic selection is the runtime
evaluation's own and is not moved by step 2: a dynamically selected
evaluation is never diagnosed at compile time.

Placement (D4405R0, "The Analysis Evaluation").  The analysis evaluation
is always the first evaluation of the assertion.  A precondition's is at
each call, in the caller, with the arguments substituted for the
parameters as for caller-side proofs (`maybe_contract_wrap_call`,
`call_analysis_semantic`; also at a virtual call, for the function it
names, and for the calls in a checked predicate and an inheriting
constructor's call); it is the stronger of the precondition's own analysis
semantic, resolved as for the callee, and what a caller-side `prove` or
`diagnose` gives it.  At a constructor call for a placeholder object (a
prvalue), a precondition naming `this` is given the slot of the temporary
the call initializes.  A temporary's destructor call is made only if no
initialization elides the temporary (`TARGET_EXPR_ELIDING_P`), which is
known once the function is complete, so its analysis evaluations wait
until then: `build_target_expr` builds the cleanup under a
`contract_temporary_cleanup_sentinel`, the call's object argument holds
`.CONTRACT_PENDING_ANALYSIS` in their place, and when the function is
genericized an elided temporary's are dropped and every other is made
(`contract_resolve_pending_analysis`; also when a call is gimplified) --
in the function that built the call, which may defer them to a proof
shadow, or else in the context it was built in (a namespace-scope
variable's, at namespace scope, though the static initialization function
makes the call), decided at once.  The EH-only cleanup of a subobject an
aggregate initialization has built makes a destructor call that is
potentially invoked whether or not anything after the subobject can throw,
so a copy of its proof markers is placed where the cleanup becomes active
(`contract_potential_cleanup_markers`, from `maybe_push_temp_cleanup`): a
shadow that removes the unreachable cleanup still analyzes the call.  An
array's element destructor calls, in a loop, are decided at once
(`contract_no_proof_deferral_sentinel`, also for `build_vec_init`'s EH-only
cleanup).  A postcondition's and a `contract_assert`'s are in
the function body, ahead of the assertion's runtime evaluation there
(`build_contract_check`, `build_analysis_evaluation`), so the proof never
takes that check as a premise; a contract wrapper makes no analysis
evaluation.  There is no analysis evaluation during constant evaluation:
a call in a manifestly constant-evaluated expression has none -- the
initializer of a `constexpr` or `constinit` variable, a constant-expression
(a `static_assert` condition, a case label, an enumerator, a bit-field
width), a template argument, an `if constexpr` condition, each also as a
template's instantiation builds it, and whatever an instantiation of a
non-dependent expression builds in a template definition, which is only
constant-evaluated or discarded; every call in an immediate function or
in the consteval branch of `if consteval`, and every call of an immediate
function.  The parser and `pt.cc` mark such an expression of the current
function while they build it (`contract_constant_evaluation_sentinel`), so
a lambda's body defined in one, or an instantiation it causes, is not
covered (`contract_constant_evaluation_p`); so does `cp_finish_decl` while
it initializes a `constexpr` or `constinit` variable (a default or value
initialization, and its temporaries' cleanups), and
`cxx_maybe_build_cleanup` while it builds the destructor call of a
constant destruction, which only constant evaluation evaluates.  The
runtime evaluations are emitted with their own semantic
beside it (`build_runtime_contract_check`); after an analysis evaluation an
`ignore` one places no assumption.  A sound proof that is assumed
(`-fcontracts-prove-assume`) drops only the checks after it in the same
code; a callee-side check is `ignore` only when the label allows no
runtime semantic.  A postcondition analyzed with `prove` or `diagnose` is
active (`contract_active_p`); a precondition analyzed so is not, its
analysis being at the calls.

Indirect calls (D4405R0, "Taking the Address of a Function").  A call
through a pointer, a pointer to member function or a reference cannot
identify the function, so it has no analysis evaluation, and a
precondition can be proven for it only if it is tautological.  Taking the
address of a function (`cp_build_addr_expr_1`: the unary `&`, the
function-to-pointer conversion, a pointer to member, binding a reference)
is diagnosed when a precondition's labels allow no runtime semantic, so
that every call through the address needs a proof, and folding does not
prove it with nothing known of the arguments (`contract_prove` with no call
and no marker): the precondition "cannot be proven" (or "is always false")
"for a call through the address of" the function, an error under `prove`
and a `-Wcontract-unproven` warning under `diagnose`, with a note at the
precondition.  When only the configuration selects `prove` or `diagnose`,
nothing is diagnosed: that translation unit's configuration does not
determine what calls through the address are
(`contract_diagnose_address_taken`).  Converting a closure to a pointer to
function takes the address of its function call operator (for a generic
lambda, the specialization converted to), whether the pointer is to the
operator itself (a static lambda) or to the thunk calling it: the call of
the conversion function is diagnosed the same way
(`contract_diagnose_lambda_conversion`, from `build_over_call`), and the
thunk's own call of the operator, whose arguments are unknown, has no
analysis evaluation.  Not in a template, an unevaluated
operand or overload resolution's trial, nor for the address a direct call
calls (`build_addr_func` sets `contract_forming_direct_call`), nor in a
lambda's conversion function, which is defined whether or not it is used.
A default argument that takes a function's address takes it at each call
using it, but is diagnosed where it is written, once, as its semantic
constraints are checked there: the conversion to the parameter's type
that `check_default_argument` makes, inside an unevaluated operand, is
diagnosed (`contract_checking_default_argument`), and the one each call
makes is not (`convert_default_arg`, `contract_using_default_argument`); a
template's is diagnosed as it is instantiated.  The runtime destroys an
object of static or thread storage duration by a call through its
destructor's address, so its definition is diagnosed the same way, with a
note naming the object (`contract_diagnose_static_destructor`, from
`expand_static_init`; not for a `constexpr` object with constant
destruction), and so is the creation of a temporary whose lifetime is
extended to static or thread storage duration, with a note naming the
extending variable (from `set_up_extended_ref_temp`).

The C front end knows a function's preconditions only where it is
defined, so its analysis evaluations are all in the function body, first,
the runtime check after them (`c_diagnose_unproven_contract`,
`c_maybe_check_postconditions`).

The label facet: `analysis_semantic_label`, a control object whose
`analysis_semantic()` is callable on a const object with no argument and
converts to `evaluation_semantic`; its value must be a constant `ignore`,
`diagnose` or `prove`, or the program is ill-formed
(`label_analysis_semantic`, `p4405-analysis-facet-invalid.C`).  A combined
label models it when either side does and returns the stronger of the two.
libstdc++'s `terminating` also allows `prove`, and `symbolic` `diagnose`
and `prove`; `all()` and the `never_` labels already hold every
enumerator.

Tests: `p4405-analysis-*.C` (configuration errors, the flag, the opt-in
lookups, labels, the facet, `compute_semantic`, the placement at calls and
in the body, the order, the runtime evaluations beside the analysis, the
drops, taking the address, including by converting a lambda, by
passing a function to `std::invoke`, `std::function`, `std::thread` or
`std::bind`, and in a default argument or by the destruction of a static object or
temporary, with the analyzed per-element destructor calls of a static
array at -O0 and -O2, the calls in constant-evaluated
contexts, a constructor's `this` at a prvalue, elided and materialized
temporaries, and the label x configuration matrix: every library label, an
`analysis_semantic` facet, combined labels and `compute_semantic` results
under each analysis and runtime configuration, by flags and by a
configuration file, for the diagnostics and the runtime evaluations),
`c-p4405-analysis-*.c`, and libstdc++'s
`p4405_analysis_semantic_label.cc`; `p4405-constexpr-enforce.C` and
`p4405-constexpr-matrix-*.C` (constant evaluation's enforce and observe
under each label, with no analysis evaluation at the constant-evaluated
calls); `p4405-paper-*.C`, D4405R0's examples with their
comments; the stage 1 to 3 tests that pinned a
precondition's `prove` or `diagnose` in the callee now expect it at a
call.

Most of the change teaches existing machinery, and those lines sit in the
commits that wrote it: the configuration's parsing, matching and resolution
and the split-set helpers are `2400-p3595`'s (`contracts-config.h`/`.cc`
and `contracts-config-query.h` are its files), as are
`ensure_evaluation_semantic`, `resolve_caller_semantic`,
`transform_semantic` and the semantic-slot helpers; the compile-time
`compute_semantic` path, now `apply_compute_semantic_split`, is
`4220-p3400-facet-compute-semantic`'s; the `terminating` and `symbolic` sets
are `4200-p3400-core`'s, and the no-flag warning `4900-p4405`'s; so are the
edits of the placement inside functions those commits wrote
(`maybe_contract_wrap_call`'s tuple, `contract_active_p`,
`diagnose_caller_side_proving`, the C parser's), and the runtime half of
`build_contract_check`, now `build_runtime_contract_check`.  This commit
holds what is the analysis evaluation's alone: the flags, their
documentation, the facet and its concept, the combined label's
`analysis_semantic`, `ensure_analysis_semantic`, the placement at calls and
in the body, the constant-evaluation sentinel and its uses in the parser
and `pt.cc`, the address-of rule and its hooks (`call.cc`, `decl.cc`,
`typeck.cc`), and the tests; `std.cc.in`'s export of the concept is
`4200-p3400-core`'s, with the rest of the labels' block.

## Compile gap

**Compile gap: earlier commits call into this one.**  `2400-p3595`'s
`contract_config_init` reads `flag_contract_analysis_semantic`, which this
commit's `c.opt` record defines, and its `ensure_evaluation_semantic` calls
`label_analysis_semantic`, defined here; so from `2400-p3595` until this
commit neither `cc1` nor `cc1plus` compiles.  The calls cannot be cut out:
they are lines inside those functions, and the semantics they compute run
through the whole resolution.

**Compile gap: `build_contract_check` is defined here.**  The runtime
evaluation it calls keeps its place in `2400-p3595`, as
`build_runtime_contract_check`; until this commit `cp-gimplify.cc`'s call
of `build_contract_check` has no definition, and `4900-p4405`'s caller-side
proofs call `call_analysis_semantic`, `prepare_call_analysis` and
`contract_constant_evaluation_p`, defined here.

## Contents

- gcc/c-family/c.opt : /^fcontract-analysis-semantic=$/
- gcc/c-family/c.opt.urls : /^fcontract-analysis-semantic=$/
- gcc/doc/invoke.texi : #1:0, /^@opindex fcontract-analysis-semantic$/, /^Set the semantic of the analysis evaluation of every contract assertion$/, /^contract configuration entry @code\{@\{"match": @\{"analysis": true@\},$/, /^configuration options\.  Requires @option\{-fcontracts-p4405\}; without it,$/, /^contract assertions too\.$/, /^Under @option\{-fcontracts-p4405\}, @code\{prove\} and @code\{diagnose\}$/, /^this option selects\.  A @code\{prove\} or @code\{diagnose\} selected for any$/, /^given to the analysis evaluation instead, which takes the stronger of it$/, /^best fit described for @option\{-fcontract-evaluation-semantic\} applies to$/, /^evaluations never reach @code\{prove\} or @code\{diagnose\}: they are limited$/, /^to the other semantics the label allows, or to @code\{ignore\} when it allows$/, /^analysis evaluation\.$/, /^The analysis evaluation is the first evaluation of a contract assertion,$/, /^so a proof never relies on the assertion's own check\.  For a precondition$/, /^whatever the semantic of the assertion's other evaluations, which are$/, /^at a call, a precondition's analysis evaluation takes place in the function$/, /^immediate function, or of one -- has none\.$/, /^A call through a pointer to a function, a pointer to member function or a$/, /^reference to a function has no analysis evaluation: it cannot identify the$/, /^a function, by the @code\{&\} operator, the function-to-pointer conversion,$/, /^forming a pointer to member function or binding a reference to it, or$/, /^its function call operator\), is therefore diagnosed when a precondition of$/, /^determine what such calls are\.  The$/, /^function, so a precondition can be proven for it only if it is$/, /^tautological\.  Taking the address of$/, /^the function allows no semantic other than @code\{prove\} and$/, /^@code\{diagnose\}, so that every call through the address needs a proof,$/, /^unless folding proves the precondition true with nothing known of the$/, /^arguments: as a precondition that ``cannot be proven'' \(or ``is always$/, /^false''\) ``for a call through the address of'' the function, an error when$/, /^its analysis evaluation is @code\{prove\} and the warning$/, /^@option\{\-Wcontract\-unproven\} when it is @code\{diagnose\}\.  Taking the$/, /^address is not itself ill\-formed\.  A precondition that the configuration$/, /^alone gives a @code\{prove\} or @code\{diagnose\} analysis evaluation is not$/, /^diagnosed there, and calls through the address are not analyzed: the$/, /^configuration of the translation unit taking the address does not$/, /^definition of an object of static or thread storage duration is treated$/, /^as taking the address of its destructor, which the implementation calls at$/, /^exit or when the thread ends, and is diagnosed the same way, unless the$/, /^object is @code\{constexpr\} and has constant destruction \(an object that is$/, /^not @code\{constexpr\} is destroyed through the address whatever its$/, /^makes for each element of such an array are ordinary calls, and their$/, /^analysis evaluations are diagnosed as well\.  A virtual$/, /^call analyzes the preconditions of the function it names\.  A default$/, /^argument that takes the address of a function is diagnosed where it is$/, /^written, once \(a template's, where it is instantiated\), not at the calls$/, /^The destructor call of a temporary that an initialization elides \(a$/, /^prvalue that initializes an object directly\) is never made, and has no$/, /^In a contract configuration \(@option\{-fcontract-configuration\}\), an entry$/, /^both @code\{"analysis": true\} and @code\{"constexpr": true\} never matches\.$/, /^evaluation is never analyzed: an entry with @code\{"constexpr": true\} \(and$/, /^constant evaluation reaches otherwise -- from an entry without the key, a$/
- gcc/cp/contracts.cc : @build_analysis_semantic_call, /^label_analysis_semantic \(tree contract, location_t loc\)$/, /^ensure_analysis_semantic \(tree contract, tree fndecl,$/, /^static contract_query make_contract_query \(tree, tree\);$/, /^prepare_call_analysis \(tree callee\)$/, /^call_analysis_semantic \(tree contract, tree callee,$/, /^bool contract_forming_direct_call;$/, @contract_diagnose_lambda_conversion:0, @contract_diagnose_lambda_conversion:1, @contract_constant_evaluation_active, @contract_constant_evaluation_sentinel, @contract_constant_evaluation_p:0, @contract_diagnose_address_taken, @inform_static_destructor_address, @contract_diagnose_static_destructor, @maybe_contract_wrap_call:2, @maybe_contract_wrap_call:4, @build_contract_assumption:1, @contract_analyzed_here_p, @build_analysis_evaluation:0, #223:3, @contract_analyze_call, /^struct GTY\(\(\)\) contract_pending_analysis$/, /^  \*contract_pending_analyses;$/, /^static GTY\(\(\)\) vec<contract_pending_analysis, va_gc>$/, @contract_temporary_cleanup_active, @contract_temporary_cleanup_sentinel, @contract_temporary_cleanup_p, @contract_call_analyzed_p, @contract_defer_call_analysis, @contract_pending_analysis, @contract_drop_pending_analyses_r, @contract_drop_pending_analyses, @contract_resolve_pending_analysis, @find_unsafe_copy_r, @contract_potential_cleanup_markers_r, @contract_no_proof_deferral_active, @contract_no_proof_deferral_sentinel, @contract_no_proof_deferral_p, @contract_potential_cleanup_markers, /^static tree build_runtime_contract_check \(tree,$/, /^  tree analysis_eval = build_analysis_evaluation \(contract, analysis\);$/, /^  \/\* D4405: an ignore evaluation after a prove or diagnose analysis$/
- gcc/cp/contracts.h : @ensure_analysis_semantic, @contract_diagnose_address_taken, @contract_diagnose_static_destructor, @contract_forming_direct_call, @contract_checking_default_argument, @contract_diagnose_lambda_conversion, @contract_constant_evaluation_sentinel, @contract_temporary_cleanup_sentinel, @contract_drop_pending_analyses, @contract_no_proof_deferral_sentinel, @contract_potential_cleanup_markers
- gcc/cp/typeck.cc : /contract_diagnose_address_taken/
- gcc/cp/call.cc : /contract_forming_direct_call/, /contract_diagnose_lambda_conversion/, /contract_using_default_argument/, /contract_diagnose_static_destructor/
- gcc/cp/decl.cc : /contract_checking_default_argument/, /contract_diagnose_static_destructor/, /contract_constant_evaluation_sentinel cce/, /else if \(constant_destruction\)/
- gcc/cp/parser.cc : /contract_constant_evaluation_sentinel/
- gcc/cp/tree.cc : /contract_temporary_cleanup_sentinel tcs/
- gcc/cp/typeck2.cc : /contract_no_proof_deferral_sentinel/, /contract_potential_cleanup_markers/, /^      if \(markers\)$/
- gcc/cp/init.cc : /contract_no_proof_deferral_sentinel nd/
- gcc/cp/cp-gimplify.cc : /contract_resolve_pending_analysis/, /contract_drop_pending_analyses/
- gcc/cp/constexpr.cc : /IFN_CONTRACT_PENDING_ANALYSIS/
- gcc/internal-fn.def : /^DEF_INTERNAL_FN \(CONTRACT_PENDING_ANALYSIS, /
- gcc/internal-fn.h : /^extern void expand_CONTRACT_PENDING_ANALYSIS /
- gcc/internal-fn.cc : /^expand_CONTRACT_PENDING_ANALYSIS /
- gcc/cp/pt.cc : /contract_constant_evaluation_sentinel/
- libstdc++-v3/include/std/contracts : @labels:1, @labels:3
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-constexpr-enforce.* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-constexpr-matrix-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-* : *
- gcc/testsuite/gcc.dg/contracts/c-p4405-analysis-* : *
- libstdc++-v3/testsuite/18_support/contracts/p4405_analysis_semantic_label.cc : *
