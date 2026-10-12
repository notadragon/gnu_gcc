# Open Upstream Bugs

Bugs found during this implementation that reproduce on stock upstream GCC,
independent of anything in this branch.  Each row links to a self-contained,
report-ready writeup plus a reproducer.  A row is removed -- and its file
deleted -- once the bug is fixed on upstream master, regardless of who fixed
it or whether it was ever formally filed.

Where the fix that closed it was **ours**, submitted to gcc-patches from this
work, the correspondence survives in
[Fixed upstream by us](#fixed-upstream-by-us) below.  Everything else leaves
no trace here, by design: upstream's own history is the record.

For what is broken on **this branch** right now, including branch-only issues
that never reproduce upstream, see
[`../open-issues/README.md`](../open-issues/README.md).

## What the columns mean

**Upstream Link** distinguishes three states, and the difference matters: a
link means a PR exists; `None found (searched <date>)` means Bugzilla was
actually searched and nothing matched; `UNKNOWN` means nobody has looked yet.
Conflating the last two is how GCC-28 stayed invisible.

**A trailing `*` marks a PR we filed ourselves** -- specifically, one whose
Bugzilla `creator` is `berne@notadragon.com`, not merely one filed by someone
connected to this work.  PR124486 is a contracts collaborator's and carries
no `*`.  Everything without one was filed by somebody else and found
afterwards, which is the common case here: of the 40 PRs linked in the
table below, 28 are other people's.  On our own PR we can comment freely and
are expected to answer questions; on someone else's the useful contribution
is usually a measurement or a confirmation, not a retelling.

**A trailing `+ comment pending` means a drafted Bugzilla comment is
waiting** in that row's writeup, under a `## Pending upstream comment
(PRnnnnnn)` heading, for the user to decide whether to post it.  The user
posts it; nobody else does.  Remove the marker, and the section, once the
comment is posted or dropped.

**Status** is about **this branch**, not upstream.  Every row here is open
upstream -- that is the entry criterion, and a row is deleted when upstream
fixes it -- so saying so again would carry no information.  `Fixed here`
means this branch does not reproduce the defect; `Open` means it does, and
those rows also appear in [`../open-issues/README.md`](../open-issues/README.md).

**Fixed By** names the commit of this branch that resolves the row.  It is
written in the source as `{{<pseudo-commit-id>}}`, the stable id of an entry
under [`../branch-history/`](../branch-history/), and the generator rewrites
it into a commit link when the branch is built -- a git hash written here by
hand would be wrong within a day, because the branch is regenerated and every
hash on it changes.  Several ids mean the fix is genuinely split and no single
commit carries it.

**A blank `Fixed By` is not "unfixed".**  It means no single commit is *about*
this defect -- the change that resolves it rides inside a commit that exists
for another reason, and linking a 19,000-line runtime library would tell a
reader nothing about the bug.  Read `Status` for whether it is fixed, and the
writeup for where the change actually lives.

Each resolved `gcc-NN`'s own file repeats the pairing on a `**Resolved by:**`
line, so it is readable without this table.

**Keywords** are recorded in each writeup on a `Keywords (ours)` line and are
deliberately not submitted: the enter-bug form has no Keywords field, and
maintainers add them during triage, usually within minutes.

**No emphasis by capital letters, anywhere in the prose.**  Capitalising a
word to stress it -- `POSTcondition`, `NOT`, `LOCAL` -- reads as shouting,
and the lowercase spelling reads identically well; 52 such sites were removed
across nine reports on 2026-09-09 and should not come back.  The contract
kinds are spelled `precondition` and `postcondition`, always, including in
reproducer comments.  Restructure a sentence rather than bolding it.  The one
exception is a section label inside a reproducer's comments (`// CONTROL:`,
`// THE BUG:`), which is structure for scanning in one pass, not emphasis.

## The table

| Bug | Summary | Status | Fixed By | Upstream Link | Details |
|-----|---------|--------|----------|----------------|---------|
| GCC-1 | Contract on a function with a variadic parameter pack ICEs or misattributes a diagnostic | Fixed here | [c97841948897](https://github.com/notadragon/gnu_gcc/commit/c978419488977eb26b82313a0dc07af4514ba8e8) | [PR124395](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124395) (+5 duplicates -- see details) + comment pending | [gcc-01-contract-pack-ice.md](gcc-01/gcc-01-contract-pack-ice.md) |
| GCC-2 | Constant evaluator does not diagnose forming an address into an object before its constructor begins, for a virtual base | Open |  | [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (partial -- see details) | [gcc-02-constexpr-vbase-before-ctor.md](gcc-02/gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-10 | Contract predicate on a lambda capturing `this` reads the closure object as the enclosing class | Fixed here | [264ceb8fee2b](https://github.com/notadragon/gnu_gcc/commit/264ceb8fee2b8398f1517b0417d6b053ffe55d2a) | [PR127283](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127283) * | [gcc-10-lambda-this-capture-in-contract.md](gcc-10/gcc-10-lambda-this-capture-in-contract.md) |
| GCC-11 | Lambda in a non-member postcondition with a result name fails to parse | Fixed here | [e256d0835bc1](https://github.com/notadragon/gnu_gcc/commit/e256d0835bc1c8b96db807baad6b374dc77ef60f) | [PR127284](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127284) * | [gcc-11-lambda-in-postcondition-result-name.md](gcc-11/gcc-11-lambda-in-postcondition-result-name.md) |
| GCC-12 | A postcondition's result binding denotes two different objects within one predicate evaluation | Fixed here | [80e027046385](https://github.com/notadragon/gnu_gcc/commit/80e027046385da7625faff1a74f334b1f76e3a81) | [PR112794](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112794) + comment pending | [gcc-12-result-binding-denotes-two-objects.md](gcc-12/gcc-12-result-binding-denotes-two-objects.md) |
| GCC-13 | Lambda's own contract specifiers are never substituted when it's instantiated | Fixed here | [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) | [PR127287](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127287) * | [gcc-13-precondition-in-lambda-nested-in-generic-lambda.md](gcc-13/gcc-13-precondition-in-lambda-nested-in-generic-lambda.md) |
| GCC-14 | Garbage source location on a contract-capture diagnostic note | Fixed here | [a3dfc0c61b33](https://github.com/notadragon/gnu_gcc/commit/a3dfc0c61b3367de512fce241ff2774a2580603f) | [PR126041](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126041) + comment pending | [gcc-14-contract-capture-note-garbage-location.md](gcc-14/gcc-14-contract-capture-note-garbage-location.md) |
| GCC-15 | Outlined contract checks lose by-value parameter/result mutations | Fixed here | [13e89f25313b](https://github.com/notadragon/gnu_gcc/commit/13e89f25313b00fb32e69cb66b8b7f17f066d369) | [PR127289](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127289) * | [gcc-15-outlined-checks-lose-by-value-mutations.md](gcc-15/gcc-15-outlined-checks-lose-by-value-mutations.md) |
| GCC-152 | A contract predicate that exits via an exception is reported with detection mode `predicate_false` instead of `evaluation_exception` | Fixed here |  | [PR125163](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125163) | [gcc-152-throwing-predicate-detection-mode.md](gcc-152/gcc-152-throwing-predicate-detection-mode.md) |
| GCC-16 | Contract on a capturing lambda inside an instantiated template segfaults | Fixed here | [d9254c1ad651](https://github.com/notadragon/gnu_gcc/commit/d9254c1ad6518bad44c534a25eb836ef6322e0a9), [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) | [PR126038](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126038) + comment pending | [gcc-16-lambda-capture-contract-in-template.md](gcc-16/gcc-16-lambda-capture-contract-in-template.md) |
| GCC-17 | `this` accepted in the trailing return type of an explicit-object member function | Open |  | [PR127290](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127290) * | [gcc-17-this-in-xobj-declaration.md](gcc-17/gcc-17-this-in-xobj-declaration.md) |
| GCC-18 | A predicate lambda naming a namespace-scope variable does not constify it | Fixed here | [a0c096326cde](https://github.com/notadragon/gnu_gcc/commit/a0c096326cde9fadf6fc4b79a129791bf32804c7) | [PR127293](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127293) * | [gcc-18-lambda-in-predicate-global-not-constified.md](gcc-18/gcc-18-lambda-in-predicate-global-not-constified.md) |
| GCC-19 | A discarded comma operand in a postcondition is wrongly treated as odr-using a by-value parameter | Fixed here | [f9d8b9086670](https://github.com/notadragon/gnu_gcc/commit/f9d8b9086670789844a270e56b2a7f02eb66ca7c) | [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) | [gcc-19-postcondition-comma-odr-use.md](gcc-19/gcc-19-postcondition-comma-odr-use.md) |
| GCC-21 | `-std=c++26` alone does not link the contract violation handler | Fixed here |  | [PR126158](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126158) | [gcc-21-cpp26-missing-experimental-link.md](gcc-21/gcc-21-cpp26-missing-experimental-link.md) |
| GCC-22 | A parameter pack of reference type in a contract is rejected outright | Fixed here | [c85eb2edad12](https://github.com/notadragon/gnu_gcc/commit/c85eb2edad12b9ddcde010b69ec42901c24c6ec1) | [PR126878](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126878), [PR126039](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126039) + comment pending | [gcc-22-pack-reference-constification.md](gcc-22/gcc-22-pack-reference-constification.md) |
| GCC-23 | [dcl.contract.func]/6 not applied to deleted or first-declaration-defaulted functions | Fixed here | [f98a2f0d4ae0](https://github.com/notadragon/gnu_gcc/commit/f98a2f0d4ae0ae50301a1b4665b57092c8af689a) | [PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486), [PR125403](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125403) + comment pending | [gcc-23-contract-on-deleted-or-defaulted.md](gcc-23/gcc-23-contract-on-deleted-or-defaulted.md) |
| GCC-24 | A noexcept function with a contract segfaults the compiler under `-fno-enforce-eh-specs` | Fixed here | [4c6679f893a8](https://github.com/notadragon/gnu_gcc/commit/4c6679f893a8c74b81947d66624a72758176b974) | [PR127173](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127173) + comment pending | [gcc-24-noexcept-body-wrapper-assumption.md](gcc-24/gcc-24-noexcept-body-wrapper-assumption.md) |
| GCC-25 | A postcondition's result-name-introducer does not accept `identifier attribute-specifier-seq :` | Fixed here | [6d05ad4def05](https://github.com/notadragon/gnu_gcc/commit/6d05ad4def05f802b445aca9411c3136fbdaf358) | [PR125725](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125725) + comment pending | [gcc-25-postcondition-result-name-attribute.md](gcc-25/gcc-25-postcondition-result-name-attribute.md) |
| GCC-26 | A redeclaration whose parameter type is dependent escapes the postcondition const rule | Fixed here | [c97841948897](https://github.com/notadragon/gnu_gcc/commit/c978419488977eb26b82313a0dc07af4514ba8e8) | [PR127196](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127196) | [gcc-26-postcondition-redecl-dependent-param.md](gcc-26/gcc-26-postcondition-redecl-dependent-param.md) |
| GCC-27 | A friend declaration's contract is never checked against an earlier declaration of the same function (two shapes; the two-friend shape is fixed here, the namespace-scope-friend shape only when a later declaration happens to flush the redeclaration-match queue) | Open |  | [PR127291](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127291) * | [gcc-27-deferred-friend-contract-mismatch.md](gcc-27/gcc-27-deferred-friend-contract-mismatch.md) |
| GCC-28 | A member named unqualified in an explicit-object member function's contract is diagnosed with a constructor/destructor message | Fixed here | [4941823ea746](https://github.com/notadragon/gnu_gcc/commit/4941823ea7468bf0c273225fa897d3b9afb99f98) | [PR127294](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127294) * | [gcc-28-xobj-member-in-predicate-ctor-message.md](gcc-28/gcc-28-xobj-member-in-predicate-ctor-message.md) |
| GCC-30 | A coroutine's postcondition may odr-use a `const` by-value parameter | Fixed here | [2241a13aae1e](https://github.com/notadragon/gnu_gcc/commit/2241a13aae1e27b652b391e0527b13fe9b8764c4) | [PR127296](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127296) * | [gcc-30-coroutine-postcondition-const-param.md](gcc-30/gcc-30-coroutine-postcondition-const-param.md) |
| GCC-31 | A contract-assertion scope is not capture-transparent: a lambda in a predicate cannot capture the enclosing parameters, and once it can, a nested `contract_assert` naming a capture ICEs | Fixed here | [9da30bfb2887](https://github.com/notadragon/gnu_gcc/commit/9da30bfb2887ea84d7366186a2a5b777e27a47bb) | [PR117435](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117435) + comment pending | [gcc-31-lambda-capture-in-contract-predicate.md](gcc-31/gcc-31-lambda-capture-in-contract-predicate.md) |
| GCC-33 | An empty-class result binding passed to a function taking a reference ICEs at codegen | Fixed here | [80e027046385](https://github.com/notadragon/gnu_gcc/commit/80e027046385da7625faff1a74f334b1f76e3a81) | [PR125574](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125574) + comment pending | [gcc-33-class-result-binding-by-reference.md](gcc-33/gcc-33-class-result-binding-by-reference.md) |
| GCC-36 | A non-template's postcondition const-parameter diagnostic is emitted twice, byte-identical at the same location | Fixed here | [f9d8b9086670](https://github.com/notadragon/gnu_gcc/commit/f9d8b9086670789844a270e56b2a7f02eb66ca7c) | [PR127297](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127297) * | [gcc-36-postcondition-const-rule-over-reported.md](gcc-36/gcc-36-postcondition-const-rule-over-reported.md) |
| GCC-37 | A `std::source_location` that is not a class (aliased away, or a union) ICEs the contract machinery | Fixed here | [7594650e73d0](https://github.com/notadragon/gnu_gcc/commit/7594650e73d0c9da82229aaadc04b5f4f342102a) | [PR127255](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127255) | [gcc-37-source-location-alias-ice.md](gcc-37/gcc-37-source-location-alias-ice.md) |
| GCC-38 | An unrelated precondition gives `std::source_location` internal linkage when the contract is seen before the header | Fixed here | [7594650e73d0](https://github.com/notadragon/gnu_gcc/commit/7594650e73d0c9da82229aaadc04b5f4f342102a) | [PR127251](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127251) | [gcc-38-source-location-linkage.md](gcc-38/gcc-38-source-location-linkage.md) |
| GCC-39 | Explicit instantiation of a variadic template rejects a cv-qualified pack argument (not a contracts bug) | Open |  | [PR126797](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126797) | [gcc-39-explicit-inst-pack-cv.md](gcc-39/gcc-39-explicit-inst-pack-cv.md) |
| GCC-42 | A contract predicate that modifies the enclosing constant evaluation is rejected as `contract condition is not constant`, rejecting a well-formed program | Fixed here | [44cbbb7da7c3](https://github.com/notadragon/gnu_gcc/commit/44cbbb7da7c35978fc8b8296d5b6e4b5aa55dd68) | None found (searched 2026-09-11) | [gcc-42-contract-side-effect-rejected.md](gcc-42/gcc-42-contract-side-effect-rejected.md) |
| GCC-43 | An `[[assume]]` whose operand contains a side-effecting contract assertion is rejected instead of declined -- a correct refusal inside a discardable evaluation escapes as a diagnostic | Fixed here | [59a323dd3430](https://github.com/notadragon/gnu_gcc/commit/59a323dd343006974f6c3935f98b779ec7b0d83a) | None found (searched 2026-09-16) | [gcc-43-assume-operand-contract-rejected.md](gcc-43/gcc-43-assume-operand-contract-rejected.md) |
| GCC-44 | A violation handler throwing out of a postcondition leaks the returned object | Fixed here | [8ee6631e6087](https://github.com/notadragon/gnu_gcc/commit/8ee6631e608771b67957cdaa4783df4a883b666e) | [PR127414](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127414) * | [gcc-44-postcondition-throw-leaks-retval.md](gcc-44/gcc-44-postcondition-throw-leaks-retval.md) |
| GCC-45 | A function-contract-specifier binds to the parameter list that precedes it rather than to the declarator, so it is accepted on a typedef, alias, parameter or pointer; silently dropped on a trailing-return type-id; resolved in the return type's parameter scope in the classic spelling; and refused on an array declarator | Fixed here | [6c266291467c](https://github.com/notadragon/gnu_gcc/commit/6c266291467caf0591ea549f3d4efb45f5ec47c9), [8edf03f17f10](https://github.com/notadragon/gnu_gcc/commit/8edf03f17f101d4eb10a3cb6b7c78859265e972f) | [PR127572](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127572) * (PR123909 + PR94984 report it for the requires-clause) | [gcc-45-contract-bound-to-wrong-declarator.md](gcc-45/gcc-45-contract-bound-to-wrong-declarator.md) |
| GCC-47 | A requires-clause on a parameter of function type is accepted and silently dropped (not a contracts bug) | Fixed here | [8edf03f17f10](https://github.com/notadragon/gnu_gcc/commit/8edf03f17f101d4eb10a3cb6b7c78859265e972f) | [PR123909](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123909), [PR94984](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94984) (siblings on the same rule, which cover this between them; not filed separately, by decision -- nothing for this position itself, searched 2026-09-16) | [gcc-47-requires-clause-on-parameter.md](gcc-47/gcc-47-requires-clause-on-parameter.md) |
| GCC-49 | A postcondition with a result name on a function whose return type deduction fails aborts the compiler | Fixed here | [490d87a45b6e](https://github.com/notadragon/gnu_gcc/commit/490d87a45b6e570f442014e6dbe320e0abbf28e3) | [PR127450](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127450) | [gcc-49-postcondition-undeduced-result-ice.md](gcc-49/gcc-49-postcondition-undeduced-result-ice.md) |
| GCC-50 | `<contracts>` is inert below C++26, so `-fcontracts` there cannot declare a violation handler | Fixed here | [09ff4f641def](https://github.com/notadragon/gnu_gcc/commit/09ff4f641defda3833fef1fefbc80493db730986) | None found (searched 2026-09-18) | [gcc-50-contracts-header-inert-below-cxx26.md](gcc-50/gcc-50-contracts-header-inert-below-cxx26.md) |
| GCC-56 | A postcondition that reads its result name is rejected as non-constant during constant evaluation: the result name is never bound, so the static potential-constant check fails before the predicate is evaluated | Fixed here | [57c77b4568bb](https://github.com/notadragon/gnu_gcc/commit/57c77b4568bba3f2df8e2fdf674f9b9bce887246) | [PR125587](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125587) + comment pending | [gcc-56-postcondition-result-not-bound-in-constexpr.md](gcc-56/gcc-56-postcondition-result-not-bound-in-constexpr.md) |
| GCC-59 | The declared type of a user-provided `::handle_contract_violation` is not checked | Fixed here | [546b875ea518](https://github.com/notadragon/gnu_gcc/commit/546b875ea51827bf4b8d86f4a6555f2faf0f4032) | None found (searched 2026-09-25) | [gcc-59-violation-handler-type-not-checked.md](gcc-59/gcc-59-violation-handler-type-not-checked.md) |
| GCC-69 | Constant evaluation accepts `delete` through a base pointer whose destructor is not virtual | Fixed here | [a8b2518a23de](https://github.com/notadragon/gnu_gcc/commit/a8b2518a23de0f9dcdc9d3529b4b0be172e508aa) | [PR97647](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97647) | [gcc-69-constexpr-delete-nonvirtual-dtor.md](gcc-69/gcc-69-constexpr-delete-nonvirtual-dtor.md) |
| GCC-70 | Constant evaluation accepts destroying an object twice (explicit destructor call, then `delete`) | Fixed here | [a8b2518a23de](https://github.com/notadragon/gnu_gcc/commit/a8b2518a23de0f9dcdc9d3529b4b0be172e508aa) | [PR126425](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126425) + comment pending | [gcc-70-constexpr-double-destroy.md](gcc-70/gcc-70-constexpr-double-destroy.md) |
| GCC-495 | A VLA named only in a `contract_assert` inside a `[&]` lambda segfaults the compiler | Fixed here | [daed1aa78af1](https://github.com/notadragon/gnu_gcc/commit/daed1aa78af18cfdc357a7d7226e6495c30eb84c) | None found (searched 2026-09-26) | [gcc-495-contract-assert-vla-implicit-capture-ice.md](gcc-495/gcc-495-contract-assert-vla-implicit-capture-ice.md) |
| GCC-502 | A constructor-precondition / destructor-postcondition call that selects a static overload is rejected | Fixed here | [4941823ea746](https://github.com/notadragon/gnu_gcc/commit/4941823ea7468bf0c273225fa897d3b9afb99f98) | None found (searched 2026-09-26) | [gcc-502-ctor-pre-static-overload.md](gcc-502/gcc-502-ctor-pre-static-overload.md) |
| GCC-503 | Implicit captures used only in contract assertions: `this`, an outer capture made for a lambda inside the predicate, and an ICE for a generic lambda | Fixed here | [daed1aa78af1](https://github.com/notadragon/gnu_gcc/commit/daed1aa78af18cfdc357a7d7226e6495c30eb84c) | None found (searched 2026-09-26) | [gcc-503-lambda-contract-only-capture.md](gcc-503/gcc-503-lambda-contract-only-capture.md) |
| GCC-596 | A lambda in one member's precondition plus a bad result name in a member template's postcondition crash after the error | Fixed here | (not identified) | None found (searched 2026-09-30) | [gcc-596-member-lambda-pre-template-post-ice.md](gcc-596/gcc-596-member-lambda-pre-template-post-ice.md) |
| GCC-607 | With `-fcontracts-client-check=pre`/`all`, a constexpr call with a precondition is not a constant expression (non-constexpr caller-side wrapper) | Fixed here | [7017fb358e8f](https://github.com/notadragon/gnu_gcc/commit/7017fb358e8f65f0a84cfce8de3a66220ef6b014), [12ed14c4ed52](https://github.com/notadragon/gnu_gcc/commit/12ed14c4ed52bfc4ee0ac5e3eb7a71cae4c24eb7) | None found (searched 2026-09-30) | [gcc-607-constexpr-caller-side-wrapper.md](gcc-607/gcc-607-constexpr-caller-side-wrapper.md) |
| GCC-608 | A defaulting redeclaration that adds a contract (`S::~S () pre (true) = default;`) crashes after its error | Fixed here | (not identified) | None found; likely a sibling of PR125403 | [gcc-608-defaulted-dtor-adds-contract.md](gcc-608/gcc-608-defaulted-dtor-adds-contract.md) |
| GCC-618 | A const local's constant-initialization attempt checks contracts: a precondition that is not constant, or false, in an initializer that is not constant anyway is a hard error instead of leaving the variable to run-time initialization | Fixed here | [7890267482b8](https://github.com/notadragon/gnu_gcc/commit/7890267482b822a72b6828da9d307570d9360f00) | None found (searched 2026-09-29) | [gcc-618-const-local-init-checks-contracts.md](gcc-618/gcc-618-const-local-init-checks-contracts.md) |
| GCC-620 | A contract violation found while constant-evaluating an array bound is diagnosed twice, byte-identical at the same location | Fixed here |  | None found (searched 2026-09-29) | [gcc-620-array-bound-contract-diagnosed-twice.md](gcc-620/gcc-620-array-bound-contract-diagnosed-twice.md) |
| GCC-630 | A friend function's contract predicate does not find members of the class it is declared in ("not declared in this scope"), whether the friend is defined in the class or outside it | Fixed here | [23a2d1ec697b](https://github.com/notadragon/gnu_gcc/commit/23a2d1ec697b8ef225a72a52072d5de4fa57d966) | None found (searched 2026-10-01) | [gcc-630-friend-contract-class-scope.md](gcc-630/gcc-630-friend-contract-class-scope.md) |
| GCC-632 | A variable declared in a statement expression in a lambda's precondition or postcondition ICEs in `gimplify_var_or_parm_decl` | Fixed here | [e9e73c8f9403](https://github.com/notadragon/gnu_gcc/commit/e9e73c8f940326dcbaf782305ca41519788df99e) | None found (searched 2026-10-01) | [gcc-632-stmt-expr-local-lambda-contract.md](gcc-632/gcc-632-stmt-expr-local-lambda-contract.md) |
| GCC-633 | A generic lambda whose precondition names one of its captures ICEs in `tsubst_expr` | Fixed here | [85b191f9bff7](https://github.com/notadragon/gnu_gcc/commit/85b191f9bff73ee6f94ed432b068d6daa0f88a7a) | None found (searched 2026-10-02) | [gcc-633-generic-lambda-pre-capture.md](gcc-633/gcc-633-generic-lambda-pre-capture.md) |
| GCC-634 | A fold over a captured pack in a lambda's precondition is rejected ("could not convert") and then ICEs; on this branch it ICEs | Fixed here | [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) | None found (searched 2026-10-02) | [gcc-634-lambda-pre-fold-capture.md](gcc-634/gcc-634-lambda-pre-fold-capture.md) |
| GCC-641 | A lambda in a contract predicate may modify `*this` through a captured `this`, where `this` is a pointer to const | Fixed here | [c8b2824664c5](https://github.com/notadragon/gnu_gcc/commit/c8b2824664c5b9c941558d09bf7ff3bea6e7b9bb) | None found (searched 2026-10-06) | [gcc-641-lambda-this-modifies-object.md](gcc-641/gcc-641-lambda-this-modifies-object.md) |
| GCC-644 | A local named in `typeid` of a polymorphic glvalue in a lambda with no capture-default is not captured and not diagnosed (a GCC 17 regression; not a contracts bug) | Open |  | None found (searched 2026-10-06) | [gcc-644-typeid-polymorphic-capture.md](gcc-644/gcc-644-typeid-polymorphic-capture.md) |
| GCC-646 | A function template declared in a precompiled header and instantiated after it loses its contract assertions (depends on the `.gch` build) | Fixed here | [5c28775fe2b9](https://github.com/notadragon/gnu_gcc/commit/5c28775fe2b9bf0141ef5ad456b449f301202b69) | None found (searched 2026-10-06) | [gcc-646-pch-template-contracts.md](gcc-646/gcc-646-pch-template-contracts.md) |
| GCC-647 | A redeclaration after a precompiled header is checked as if the header's declarations had no contracts | Fixed here | [5c28775fe2b9](https://github.com/notadragon/gnu_gcc/commit/5c28775fe2b9bf0141ef5ad456b449f301202b69) | None found (searched 2026-10-06) | [gcc-647-pch-redecl-contracts.md](gcc-647/gcc-647-pch-redecl-contracts.md) |
| GCC-648 | A parameter pack used only in a lambda's contract assertion is implicitly captured without a diagnostic | Fixed here | [a3dfc0c61b33](https://github.com/notadragon/gnu_gcc/commit/a3dfc0c61b3367de512fce241ff2774a2580603f) | None found (searched 2026-10-06) | [gcc-648-contract-only-capture-pack.md](gcc-648/gcc-648-contract-only-capture-pack.md) |
| GCC-649 | A constant read in a lambda's contract assertion is diagnosed as "not implicitly captured by a contract assertion" | Fixed here | [a3dfc0c61b33](https://github.com/notadragon/gnu_gcc/commit/a3dfc0c61b3367de512fce241ff2774a2580603f) | None found (searched 2026-10-06) | [gcc-649-contract-only-capture-constant.md](gcc-649/gcc-649-contract-only-capture-constant.md) |
| GCC-650 | A parameter named only in a discarded member access in a postcondition is required to be const | Fixed here | [f9d8b9086670](https://github.com/notadragon/gnu_gcc/commit/f9d8b9086670789844a270e56b2a7f02eb66ca7c) | [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) (related) | [gcc-650-postcondition-discarded-member-access.md](gcc-650/gcc-650-postcondition-discarded-member-access.md) |
| GCC-651 | An unexpanded parameter pack in a contract predicate is not diagnosed in the template | Fixed here | [cd05b8e1d78c](https://github.com/notadragon/gnu_gcc/commit/cd05b8e1d78c0653e6049a9c4407fef7c1dc5d1b) | None found (searched 2026-10-06) | [gcc-651-contract-unexpanded-pack.md](gcc-651/gcc-651-contract-unexpanded-pack.md) |
| GCC-652 | A statement expression's variable in a generic lambda's precondition is rejected when the operator() is instantiated | Fixed here | [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) | None found (searched 2026-10-06) | [gcc-652-stmt-expr-local-generic-lambda-contract.md](gcc-652/gcc-652-stmt-expr-local-generic-lambda-contract.md) |
| GCC-656 | A lambda in a function template's precondition cannot capture the template's parameter pack | Fixed here | [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) | None found (searched 2026-10-06) | [gcc-656-predicate-lambda-pack-capture.md](gcc-656/gcc-656-predicate-lambda-pack-capture.md) |
| GCC-663 | A function template's parameter named only as the object of a discarded member access in a postcondition is required to be const on the definition | Fixed here | [f9d8b9086670](https://github.com/notadragon/gnu_gcc/commit/f9d8b9086670789844a270e56b2a7f02eb66ca7c) | None found (searched 2026-10-07); related [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) | [gcc-663-postcondition-discarded-member-access-template.md](gcc-663/gcc-663-postcondition-discarded-member-access-template.md) |
| GCC-682 | `noexcept` of an initializer list ignores a potentially-throwing default constructor run by its array filler (`noexcept(A{})` with `struct A { S s[2]; }`; `A{{}}` and `S[2]{}` a GCC 14 regression) | Fixed here | [5101bfa38a32](https://github.com/notadragon/gnu_gcc/commit/5101bfa38a3237a61c72ff959d3dcfa8c84c1ed8) | None found (searched 2026-10-09) | [gcc-682-noexcept-array-filler.md](gcc-682/gcc-682-noexcept-array-filler.md) |
| GCC-683 | `noexcept` of a delete-expression whose operand is a null pointer constant ignores a potentially-throwing destructor | Open |  | [PR65174](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65174) | [gcc-683-noexcept-delete-null.md](gcc-683/gcc-683-noexcept-delete-null.md) |
| GCC-689 | A potentially-throwing contract predicate checked under quick_enforce draws a bogus `-Wmaybe-uninitialized` warning about a compiler temporary | Fixed here | [546b875ea518](https://github.com/notadragon/gnu_gcc/commit/546b875ea51827bf4b8d86f4a6555f2faf0f4032) | [PR125163](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125163) (comment 2 only; no PR of its own, searched 2026-10-10) | [gcc-689-quick-enforce-throwing-predicate-uninit.md](gcc-689/gcc-689-quick-enforce-throwing-predicate-uninit.md) |
| GCC-706 | A contract predicate spanning a line of only blanks ICEs in `get_source_text_between` | Fixed here | [e54d8596aef6](https://github.com/notadragon/gnu_gcc/commit/e54d8596aef660397ecdc0c4deacfbcfbf85fd89) | [PR127857](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127857) | [gcc-706-contract-comment-blank-line.md](gcc-706/gcc-706-contract-comment-blank-line.md) |
| GCC-707 | A contract predicate that begins or ends inside a macro expansion gets a wrong comment, which can include unrelated source | Fixed here | [1db751868103](https://github.com/notadragon/gnu_gcc/commit/1db75186810311eee8bc271bcf0c37d4acaa3128) | None found (searched 2026-10-12) | [gcc-707-contract-comment-macro.md](gcc-707/gcc-707-contract-comment-macro.md) |
| GCC-504 | A hidden friend with a precondition in a class template: ICE, or a dependent type left in the predicate | Fixed here | [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) | None found (searched 2026-09-26) | [gcc-504-hidden-friend-template-pre.md](gcc-504/gcc-504-hidden-friend-template-pre.md) |
| GCC-506 | A lambda whose precondition names a member, in a default member initializer of a class template, is rejected (ICE on this branch) | Fixed here | [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) | [PR127287](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127287) * (same defect, member-through-`this` face) + comment pending | [gcc-506-nsdmi-lambda-contract-this.md](gcc-506/gcc-506-nsdmi-lambda-contract-this.md) |
| GCC-507 | Implicit member access inside a lambda in a constructor precondition is accepted | Fixed here | [4941823ea746](https://github.com/notadragon/gnu_gcc/commit/4941823ea7468bf0c273225fa897d3b9afb99f98) | None found (searched 2026-09-26) | [gcc-507-ctor-pre-member-in-lambda.md](gcc-507/gcc-507-ctor-pre-member-in-lambda.md) |
| GCC-510 | A precompiled header's contract evaluation semantic silently overrides the TU's | Fixed here | [056c791341cb](https://github.com/notadragon/gnu_gcc/commit/056c791341cb9a3d86ef12e2b4da69cc8aa16e44) | None found (searched 2026-09-26) | [gcc-510-pch-contract-semantic-overrides-tu.md](gcc-510/gcc-510-pch-contract-semantic-overrides-tu.md) |
| GCC-517 | A postcondition result name that conflicts with a parameter is diagnosed only in a checked definition | Fixed here | [de4406f666d1](https://github.com/notadragon/gnu_gcc/commit/de4406f666d1d7d4590b3c5427907d39dcb5bb63) | None found (searched 2026-09-26) | [gcc-517-result-name-shadows-parameter.md](gcc-517/gcc-517-result-name-shadows-parameter.md) |
| GCC-518 | A redeclaration that adds or drops a postcondition's result-name-introducer is accepted | Fixed here | [de4406f666d1](https://github.com/notadragon/gnu_gcc/commit/de4406f666d1d7d4590b3c5427907d39dcb5bb63) | None found (searched 2026-09-26) | [gcc-518-redecl-result-name-presence.md](gcc-518/gcc-518-redecl-result-name-presence.md) |
| GCC-521 | Redeclaration contract predicates are compared after constant folding | Fixed here | [de4406f666d1](https://github.com/notadragon/gnu_gcc/commit/de4406f666d1d7d4590b3c5427907d39dcb5bb63) | None found (searched 2026-09-26) | [gcc-521-redecl-condition-folded.md](gcc-521/gcc-521-redecl-condition-folded.md) |
| GCC-523 | Contract wrappers drop the callee's `noreturn` and `nodiscard`, and diagnostics name the wrapper | Fixed here | [7017fb358e8f](https://github.com/notadragon/gnu_gcc/commit/7017fb358e8f65f0a84cfce8de3a66220ef6b014) | None found (searched 2026-09-26) | [gcc-523-contract-wrapper-drops-attributes.md](gcc-523/gcc-523-contract-wrapper-drops-attributes.md) |
| GCC-543 | A lambda in a declaration's contract, then any default argument -> ICE | Fixed here | [4cce0358680d](https://github.com/notadragon/gnu_gcc/commit/4cce0358680d279367a28a88317a3f85f4a35a3c) | [PR127428](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127428) + comment pending | [gcc-543-contract-lambda-then-default-arg-ice.md](gcc-543/gcc-543-contract-lambda-then-default-arg-ice.md) |
| GCC-544 | A lambda in the in-class contract of a templated member function -> ICE | Fixed here | [4cce0358680d](https://github.com/notadragon/gnu_gcc/commit/4cce0358680d279367a28a88317a3f85f4a35a3c) | None found (searched 2026-09-26) | [gcc-544-template-decl-contract-lambda-ice.md](gcc-544/gcc-544-template-decl-contract-lambda-ice.md) |
| GCC-551 | The contract on a block-scope first declaration is dropped (or reported as "added") | Fixed here | [80065260b084](https://github.com/notadragon/gnu_gcc/commit/80065260b08455b698a22017c35a3d499eae5376) | None found (searched 2026-09-26) | [gcc-551-block-scope-first-decl-contract.md](gcc-551/gcc-551-block-scope-first-decl-contract.md) |
| GCC-612 | A templated function declared with a contract whose lambda captures a parameter, defined without repeating it -> ICE on instantiation | Fixed here | [36d7bb4c0f3c](https://github.com/notadragon/gnu_gcc/commit/36d7bb4c0f3c7545506d1ea65b0e5a48bf3f3288) | None found (searched 2026-09-26) | [gcc-612-redecl-omit-contract-lambda-ice.md](gcc-612/gcc-612-redecl-omit-contract-lambda-ice.md) |
| GCC-552 | An explicit specialization declared with a contract and defined without one loses the contract | Fixed here | [320e903ee5df](https://github.com/notadragon/gnu_gcc/commit/320e903ee5df8fdda131105f8fe6a191ab6681e4) | None found (searched 2026-09-26) | [gcc-552-explicit-spec-contract-dropped.md](gcc-552/gcc-552-explicit-spec-contract-dropped.md) |
| GCC-553 | Templates exported from a module lose their contracts | Fixed here | [70d06c3ef1af](https://github.com/notadragon/gnu_gcc/commit/70d06c3ef1afc229ab915f50454807e9ee0d9b2d) | None found (searched 2026-09-26) | [gcc-553-module-template-contract-dropped.md](gcc-553/gcc-553-module-template-contract-dropped.md) |
| GCC-569 | A null object pointer through `->*` (pointer to data member) is not checked | Fixed here | [95dd23d3644a](https://github.com/notadragon/gnu_gcc/commit/95dd23d3644a42248b5128c41586c6a162f0c971) | None found (searched 2026-09-26) | [gcc-569-ptrmem-null-object-not-checked.md](gcc-569/gcc-569-ptrmem-null-object-not-checked.md) |
| GCC-574 | A constructor-precondition / destructor-postcondition member rule rejects unevaluated uses (`sizeof(b)`) and accepts the qualified form `pre(C::b)` | Fixed here | [d474ef57d202](https://github.com/notadragon/gnu_gcc/commit/d474ef57d202a37858b2766791583a953f40716c) | None found (searched 2026-09-26) | [gcc-574-ctor-pre-member-unevaluated-and-qualified.md](gcc-574/gcc-574-ctor-pre-member-unevaluated-and-qualified.md) |
| GCC-606 | An explicit by-copy lambda capture in a contract predicate gets the constified type, so a mutable lambda cannot modify its copy | Fixed here | [68afc5ba3fbf](https://github.com/notadragon/gnu_gcc/commit/68afc5ba3fbf3608842bb36bfbaa2dca54c811ec) | None found (searched 2026-09-26) | [gcc-606-contract-explicit-copy-capture-const.md](gcc-606/gcc-606-contract-explicit-copy-capture-const.md) |
| GCC-610 | A lambda capture named in a contract crashes the compiler under -fcontract-checks-outlined | Fixed here | [36d7bb4c0f3c](https://github.com/notadragon/gnu_gcc/commit/36d7bb4c0f3c7545506d1ea65b0e5a48bf3f3288) | None found (searched 2026-09-26) | [gcc-610-outlined-lambda-capture-proxy-ice.md](gcc-610/gcc-610-outlined-lambda-capture-proxy-ice.md) |

## Fixed upstream by us

Rows that left the table above because **this branch's fix was submitted to
gcc-patches and landed**, rather than because somebody else got there first.
The writeup and the reproducer are deleted along with the row -- upstream's
commit and the test it carries are the durable record, and a second copy here
would only rot -- so what is kept is the correspondence between our id, the
PR, and the commit that closed it.

The distinction is worth the table.  A bug closed by someone else leaves us
nothing to answer for; one closed by our own patch means there is a commit in
GCC with our name on it, and a reader asking "what came of that row?" should
be able to find it without a Bugzilla search.

| Bug | Summary | Upstream Link | Upstream commit | Landed |
|-----|---------|---------------|-----------------|--------|
| GCC-9 | Nested `[[assume]]` leaks the inner operand's side effects during constant evaluation | [PR127282](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127282) * | [`7b60a368fb1`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=7b60a368fb19b33f8676482a233d389cbc47b85e) | 2026-09-14 |
| GCC-5 | Contract on a function returning a non-trivially-destructible class double-destroys the return value | [PR127281](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127281) * | [`e8207be46a6`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=e8207be46a6ebbbc079adc72c763b648de5c2da5) | 2026-09-20 |
| GCC-29 | The `__contract_assert` extension spelling ICEs in `grok_contract` | [PR127295](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127295) * | [`3bd34f8936c`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=3bd34f8936c7edb25d35bc2816a863ca902938bf) | 2026-09-20 |
| GCC-3 | A contract condition that re-calls a constexpr function already called in the same constant evaluation is wrongly rejected as non-constant | [PR125459](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125459) | [`21a88667c67`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=21a88667c67ddf1f3086b7f05dc88601f1c6b070) | 2026-09-22 |

**What this branch keeps of such a row.**  Nothing in `branch-history/`: the
fix arrives with the rebase, so the entry that used to carry it is deleted and
its lines stop appearing in the diff at all.  What can survive is coverage
upstream had no way to take -- for GCC-9 that is
[`contract-constexpr-side-effect-nested.C`](../gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect-nested.C),
which needs `-fcontracts` and so could not go with the patch.  The
contracts-free reproducer did go with it, as
`g++.dg/cpp23/attr-assume10.C`.

**A retiring row can also leave a live bug behind, and GCC-3 did.**  Its
reproducer carried four cases and its writeup cited two PRs as "the same
defect".  Upstream's fix closed three cases and PR125459; the fourth, the
postcondition one, still fails on stock and is PR125587 -- a genuinely
different code path reached through a different guard.  It is now
[GCC-56](#the-table).  Before deleting a retiring row's reproducer, run it
against stock with the fix in: a file that still diagnoses is telling you it
covered more than one defect.

## Which compilers a claim in this directory is measured against

**The current Compiler Explorer trunk nightly, and every published GCC 16
release Compiler Explorer carries** -- as of 2026-09-09 that is `16.1.0` and
`16.2.0`. Not "a trunk build" and not "whatever released g++ is to hand": the
set is fixed so that every writeup here makes the same claim about the same
compilers, and so that a reader of the eventual Bugzilla report can reproduce
it. There is no 16.0.0 to chase -- GCC's first release of a major series is
always X.1.0, and X.0.0 only ever names in-development trunk.

These all come from the Compiler Explorer binary mirror, which anyone can
download from, so that a claim here always names a compiler a reader can
obtain. **The nightly being out of date is its normal state** --
the mirror keeps only a five-day window, so refresh it before measuring
rather than after. The GCC 16 tree is whatever the mirror's newest
`opt/gcc-16.*.tar.xz` is; as of 2026-09-09 the mirror carries 16.1.0 and
16.2.0 only, so that is `gcc-16.2.0`.

Record the exact build id of each in the writeup -- `17.0.0 20260909
(experimental)`, not "trunk" -- because a claim measured against a nightly
stops being checkable the moment that nightly ages out.

## Re-measuring

`verify.sh` measures every reproducer here against both stock upstream trunk
and this branch, and prints only what has moved since the checked-in baseline
in `verify-expected.txt`:

```
./verify.sh              # report anything that moved
./verify.sh --record     # re-baseline, then review the diff
./verify.sh -v GCC-30    # show the diagnostics behind a moved digest
```

A move in the **stock** column is the event this table cares about: upstream
closing a bug is silent from here, so nothing else notices when a row stops
belonging.  A move in the **branch** column is a regression or a fix here.
The cases and their flags are in `verify-cases.txt`.
