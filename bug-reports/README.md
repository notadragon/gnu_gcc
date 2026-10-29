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

**A trailing `-- filing owed` after `None found` means a report is owed**:
the writeup carries a `## Draft upstream report` (or `## Bug Report`)
section ready to file.  The user files it; nobody else does.  Replace the
marker with the link once it is filed.

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
| GCC-1 | Contract on a function with a variadic parameter pack ICEs or misattributes a diagnostic | Fixed here | [c249d3afe881](https://github.com/notadragon/gnu_gcc/commit/c249d3afe881d49cde65e5d33ccb6e010e3f4614) | [PR124395](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124395) (+5 duplicates -- see details) + comment pending | [gcc-01-contract-pack-ice.md](gcc-01/gcc-01-contract-pack-ice.md) |
| GCC-2 | Constant evaluator does not diagnose forming an address into an object before its constructor begins, for a virtual base | Open |  | [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (partial -- see details) | [gcc-02-constexpr-vbase-before-ctor.md](gcc-02/gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-10 | Contract predicate on a lambda capturing `this` reads the closure object as the enclosing class | Fixed here | [c6cc9e9d3163](https://github.com/notadragon/gnu_gcc/commit/c6cc9e9d3163f3f01cfe4f66e1009f625bc7b285) | [PR127283](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127283) * | [gcc-10-lambda-this-capture-in-contract.md](gcc-10/gcc-10-lambda-this-capture-in-contract.md) |
| GCC-11 | Lambda in a non-member postcondition with a result name fails to parse | Fixed here | [357f15dee00d](https://github.com/notadragon/gnu_gcc/commit/357f15dee00df10fc35717e4cd32b91d9e5e478e) | [PR127284](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127284) * | [gcc-11-lambda-in-postcondition-result-name.md](gcc-11/gcc-11-lambda-in-postcondition-result-name.md) |
| GCC-12 | A postcondition's result binding denotes two different objects within one predicate evaluation | Fixed here | [3a9cc143cb14](https://github.com/notadragon/gnu_gcc/commit/3a9cc143cb14d9b9df1329d917dc8a587627fd2c) | [PR112794](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112794) + comment pending | [gcc-12-result-binding-denotes-two-objects.md](gcc-12/gcc-12-result-binding-denotes-two-objects.md) |
| GCC-13 | Lambda's own contract specifiers are never substituted when it's instantiated | Fixed here | [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009) | [PR127287](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127287) * | [gcc-13-precondition-in-lambda-nested-in-generic-lambda.md](gcc-13/gcc-13-precondition-in-lambda-nested-in-generic-lambda.md) |
| GCC-14 | Garbage source location on a contract-capture diagnostic note | Fixed here | [60072d1efe38](https://github.com/notadragon/gnu_gcc/commit/60072d1efe38f36174f3e78ac153adcb3022f3be) | [PR126041](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126041) + comment pending | [gcc-14-contract-capture-note-garbage-location.md](gcc-14/gcc-14-contract-capture-note-garbage-location.md) |
| GCC-15 | Outlined contract checks lose by-value parameter/result mutations | Fixed here | [56426e226f48](https://github.com/notadragon/gnu_gcc/commit/56426e226f487d6736412baa7d83a7cf608d16a8) | [PR127289](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127289) * | [gcc-15-outlined-checks-lose-by-value-mutations.md](gcc-15/gcc-15-outlined-checks-lose-by-value-mutations.md) |
| GCC-152 | A contract predicate that exits via an exception is reported with detection mode `predicate_false` instead of `evaluation_exception` | Fixed here |  | [PR125163](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125163) | [gcc-152-throwing-predicate-detection-mode.md](gcc-152/gcc-152-throwing-predicate-detection-mode.md) |
| GCC-16 | Contract on a capturing lambda inside an instantiated template segfaults | Fixed here | [0d871ca5faa7](https://github.com/notadragon/gnu_gcc/commit/0d871ca5faa78eabeb9af6ace2cd08291e329b49), [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009) | [PR126038](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126038) + comment pending | [gcc-16-lambda-capture-contract-in-template.md](gcc-16/gcc-16-lambda-capture-contract-in-template.md) |
| GCC-17 | `this` accepted in the trailing return type of an explicit-object member function | Open |  | [PR127290](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127290) * | [gcc-17-this-in-xobj-declaration.md](gcc-17/gcc-17-this-in-xobj-declaration.md) |
| GCC-18 | A predicate lambda naming a namespace-scope variable does not constify it | Fixed here | [2a703089cddb](https://github.com/notadragon/gnu_gcc/commit/2a703089cddb9fef43c635eeb6cd423de5e8934a) | [PR127293](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127293) * | [gcc-18-lambda-in-predicate-global-not-constified.md](gcc-18/gcc-18-lambda-in-predicate-global-not-constified.md) |
| GCC-19 | A discarded comma operand in a postcondition is wrongly treated as odr-using a by-value parameter | Fixed here | [16b8ae6840ca](https://github.com/notadragon/gnu_gcc/commit/16b8ae6840cacd49f0592ad23cbc710d951effad) | [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) | [gcc-19-postcondition-comma-odr-use.md](gcc-19/gcc-19-postcondition-comma-odr-use.md) |
| GCC-21 | `-std=c++26` alone does not link the contract violation handler | Fixed here |  | [PR126158](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126158) | [gcc-21-cpp26-missing-experimental-link.md](gcc-21/gcc-21-cpp26-missing-experimental-link.md) |
| GCC-22 | A parameter pack of reference type in a contract is rejected outright | Fixed here | [4d6403dfd36b](https://github.com/notadragon/gnu_gcc/commit/4d6403dfd36b46257eca152209d55d3aad64c87d) | [PR126878](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126878), [PR126039](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126039) + comment pending | [gcc-22-pack-reference-constification.md](gcc-22/gcc-22-pack-reference-constification.md) |
| GCC-23 | [dcl.contract.func]/6 not applied to deleted or first-declaration-defaulted functions | Fixed here | [9f09b82a8183](https://github.com/notadragon/gnu_gcc/commit/9f09b82a8183ceb7d1327ccfd6581281963229de) | [PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486), [PR125403](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125403) + comment pending | [gcc-23-contract-on-deleted-or-defaulted.md](gcc-23/gcc-23-contract-on-deleted-or-defaulted.md) |
| GCC-24 | A noexcept function with a contract segfaults the compiler under `-fno-enforce-eh-specs` | Fixed here | [3bf924c07d0f](https://github.com/notadragon/gnu_gcc/commit/3bf924c07d0f001b29a445321728a839f3d271f1) | [PR127173](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127173) + comment pending | [gcc-24-noexcept-body-wrapper-assumption.md](gcc-24/gcc-24-noexcept-body-wrapper-assumption.md) |
| GCC-25 | A postcondition's result-name-introducer does not accept `identifier attribute-specifier-seq :` | Fixed here | [63892b25fe18](https://github.com/notadragon/gnu_gcc/commit/63892b25fe1833b00f5f2fa4e0c18f835302058c) | [PR125725](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125725) + comment pending | [gcc-25-postcondition-result-name-attribute.md](gcc-25/gcc-25-postcondition-result-name-attribute.md) |
| GCC-26 | A redeclaration whose parameter type is dependent escapes the postcondition const rule | Fixed here | [c249d3afe881](https://github.com/notadragon/gnu_gcc/commit/c249d3afe881d49cde65e5d33ccb6e010e3f4614) | [PR127196](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127196) | [gcc-26-postcondition-redecl-dependent-param.md](gcc-26/gcc-26-postcondition-redecl-dependent-param.md) |
| GCC-27 | A friend declaration's contract is never checked against an earlier declaration of the same function (two shapes; the two-friend shape is fixed here, the namespace-scope-friend shape only when a later declaration happens to flush the redeclaration-match queue) | Open |  | [PR127291](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127291) * | [gcc-27-deferred-friend-contract-mismatch.md](gcc-27/gcc-27-deferred-friend-contract-mismatch.md) |
| GCC-28 | A member named unqualified in an explicit-object member function's contract is diagnosed with a constructor/destructor message | Fixed here | [bff299b3f396](https://github.com/notadragon/gnu_gcc/commit/bff299b3f396e92d15db7b7f99a48103d042753b) | [PR127294](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127294) * | [gcc-28-xobj-member-in-predicate-ctor-message.md](gcc-28/gcc-28-xobj-member-in-predicate-ctor-message.md) |
| GCC-30 | A coroutine's postcondition may odr-use a `const` by-value parameter | Fixed here | [b51689042e00](https://github.com/notadragon/gnu_gcc/commit/b51689042e00b83bc2a78e03315b1b235008848e) | [PR127296](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127296) * | [gcc-30-coroutine-postcondition-const-param.md](gcc-30/gcc-30-coroutine-postcondition-const-param.md) |
| GCC-31 | A contract-assertion scope is not capture-transparent: a lambda in a predicate cannot capture the enclosing parameters, and once it can, a nested `contract_assert` naming a capture ICEs | Fixed here | [cfefcc9ac72e](https://github.com/notadragon/gnu_gcc/commit/cfefcc9ac72e8eb7a93d857709350eaa9835552d) | [PR117435](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117435) + comment pending | [gcc-31-lambda-capture-in-contract-predicate.md](gcc-31/gcc-31-lambda-capture-in-contract-predicate.md) |
| GCC-33 | An empty-class result binding passed to a function taking a reference ICEs at codegen | Fixed here | [3a9cc143cb14](https://github.com/notadragon/gnu_gcc/commit/3a9cc143cb14d9b9df1329d917dc8a587627fd2c) | [PR125574](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125574) + comment pending | [gcc-33-class-result-binding-by-reference.md](gcc-33/gcc-33-class-result-binding-by-reference.md) |
| GCC-36 | A non-template's postcondition const-parameter diagnostic is emitted twice, byte-identical at the same location | Fixed here | [16b8ae6840ca](https://github.com/notadragon/gnu_gcc/commit/16b8ae6840cacd49f0592ad23cbc710d951effad) | [PR127297](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127297) * | [gcc-36-postcondition-const-rule-over-reported.md](gcc-36/gcc-36-postcondition-const-rule-over-reported.md) |
| GCC-37 | A `std::source_location` that is not a class (aliased away, or a union) ICEs the contract machinery | Fixed here | [b9b6ca456d33](https://github.com/notadragon/gnu_gcc/commit/b9b6ca456d335dd651c38c5e950f27386264df80) | [PR127255](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127255) | [gcc-37-source-location-alias-ice.md](gcc-37/gcc-37-source-location-alias-ice.md) |
| GCC-38 | An unrelated precondition gives `std::source_location` internal linkage when the contract is seen before the header | Fixed here | [b9b6ca456d33](https://github.com/notadragon/gnu_gcc/commit/b9b6ca456d335dd651c38c5e950f27386264df80) | [PR127251](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127251) | [gcc-38-source-location-linkage.md](gcc-38/gcc-38-source-location-linkage.md) |
| GCC-39 | Explicit instantiation of a variadic template rejects a cv-qualified pack argument (not a contracts bug) | Open |  | [PR126797](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126797) | [gcc-39-explicit-inst-pack-cv.md](gcc-39/gcc-39-explicit-inst-pack-cv.md) |
| GCC-42 | A contract predicate that modifies the enclosing constant evaluation is rejected as `contract condition is not constant`, rejecting a well-formed program | Fixed here | [3970965ecddf](https://github.com/notadragon/gnu_gcc/commit/3970965ecddf47a845e48d82248ed0e95af3ad46) | None found (searched 2026-09-11) | [gcc-42-contract-side-effect-rejected.md](gcc-42/gcc-42-contract-side-effect-rejected.md) |
| GCC-43 | An `[[assume]]` whose operand contains a side-effecting contract assertion is rejected instead of declined -- a correct refusal inside a discardable evaluation escapes as a diagnostic | Fixed here | [06c5a1f1800f](https://github.com/notadragon/gnu_gcc/commit/06c5a1f1800f1b3a43e54eaf88b74db40ec0487a) | None found (searched 2026-09-16) | [gcc-43-assume-operand-contract-rejected.md](gcc-43/gcc-43-assume-operand-contract-rejected.md) |
| GCC-44 | A violation handler throwing out of a postcondition leaks the returned object | Fixed here | [d2059a6b90fa](https://github.com/notadragon/gnu_gcc/commit/d2059a6b90fa0a778ae7b33b8b4d019080862dbe) | [PR127414](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127414) * | [gcc-44-postcondition-throw-leaks-retval.md](gcc-44/gcc-44-postcondition-throw-leaks-retval.md) |
| GCC-45 | A function-contract-specifier binds to the parameter list that precedes it rather than to the declarator, so it is accepted on a typedef, alias, parameter or pointer; silently dropped on a trailing-return type-id; resolved in the return type's parameter scope in the classic spelling; and refused on an array declarator | Fixed here | [da2deb2ef334](https://github.com/notadragon/gnu_gcc/commit/da2deb2ef33471a697d2dee2dda32f6a6ae4b8a2), [5c90ce7a503b](https://github.com/notadragon/gnu_gcc/commit/5c90ce7a503b7fe231abfc14f68a466f7a4e4a5e) | [PR127572](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127572) * (PR123909 + PR94984 report it for the requires-clause) | [gcc-45-contract-bound-to-wrong-declarator.md](gcc-45/gcc-45-contract-bound-to-wrong-declarator.md) |
| GCC-47 | A requires-clause on a parameter of function type is accepted and silently dropped (not a contracts bug) | Fixed here | [5c90ce7a503b](https://github.com/notadragon/gnu_gcc/commit/5c90ce7a503b7fe231abfc14f68a466f7a4e4a5e) | [PR123909](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123909), [PR94984](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94984) (siblings on the same rule, which cover this between them; not filed separately, by decision -- nothing for this position itself, searched 2026-09-16) | [gcc-47-requires-clause-on-parameter.md](gcc-47/gcc-47-requires-clause-on-parameter.md) |
| GCC-49 | A postcondition with a result name on a function whose return type deduction fails aborts the compiler | Fixed here | [0f8d6145ade7](https://github.com/notadragon/gnu_gcc/commit/0f8d6145ade7ed2c4b0fde372fa8f34e9f73adb7) | [PR127450](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127450) | [gcc-49-postcondition-undeduced-result-ice.md](gcc-49/gcc-49-postcondition-undeduced-result-ice.md) |
| GCC-50 | `<contracts>` is inert below C++26, so `-fcontracts` there cannot declare a violation handler | Fixed here | [053d7dfd72ea](https://github.com/notadragon/gnu_gcc/commit/053d7dfd72ea94dbd18814c8e8f5d89ad8d60496) | None found (searched 2026-09-18) | [gcc-50-contracts-header-inert-below-cxx26.md](gcc-50/gcc-50-contracts-header-inert-below-cxx26.md) |
| GCC-56 | A postcondition that reads its result name is rejected as non-constant during constant evaluation: the result name is never bound, so the static potential-constant check fails before the predicate is evaluated | Fixed here | [b5a6973dddfb](https://github.com/notadragon/gnu_gcc/commit/b5a6973dddfb3e7e32b43b1b61d405b00293cf70) | [PR125587](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125587) + comment pending | [gcc-56-postcondition-result-not-bound-in-constexpr.md](gcc-56/gcc-56-postcondition-result-not-bound-in-constexpr.md) |
| GCC-59 | The declared type of a user-provided `::handle_contract_violation` is not checked | Fixed here | [9aff43d99da2](https://github.com/notadragon/gnu_gcc/commit/9aff43d99da20aaee862b674c70115817d80ec75) | None found (searched 2026-09-25) | [gcc-59-violation-handler-type-not-checked.md](gcc-59/gcc-59-violation-handler-type-not-checked.md) |
| GCC-69 | Constant evaluation accepts `delete` through a base pointer whose destructor is not virtual | Fixed here | [10ef2099e1a5](https://github.com/notadragon/gnu_gcc/commit/10ef2099e1a52c3af7d7d10bc2abbd59887182a8) | [PR97647](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97647) | [gcc-69-constexpr-delete-nonvirtual-dtor.md](gcc-69/gcc-69-constexpr-delete-nonvirtual-dtor.md) |
| GCC-70 | Constant evaluation accepts destroying an object twice (explicit destructor call, then `delete`) | Fixed here | [10ef2099e1a5](https://github.com/notadragon/gnu_gcc/commit/10ef2099e1a52c3af7d7d10bc2abbd59887182a8) | [PR126425](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126425) + comment pending | [gcc-70-constexpr-double-destroy.md](gcc-70/gcc-70-constexpr-double-destroy.md) |
| GCC-495 | A VLA named only in a `contract_assert` inside a `[&]` lambda segfaults the compiler | Fixed here | [cd4981120f7a](https://github.com/notadragon/gnu_gcc/commit/cd4981120f7a5e343ab146732e7d774e228a5eeb) | None found (searched 2026-09-26) | [gcc-495-contract-assert-vla-implicit-capture-ice.md](gcc-495/gcc-495-contract-assert-vla-implicit-capture-ice.md) |
| GCC-502 | A constructor-precondition / destructor-postcondition call that selects a static overload is rejected | Fixed here | [bff299b3f396](https://github.com/notadragon/gnu_gcc/commit/bff299b3f396e92d15db7b7f99a48103d042753b) | None found (searched 2026-09-26) | [gcc-502-ctor-pre-static-overload.md](gcc-502/gcc-502-ctor-pre-static-overload.md) |
| GCC-503 | Implicit captures used only in contract assertions: `this`, an outer capture made for a lambda inside the predicate, and an ICE for a generic lambda | Fixed here | [cd4981120f7a](https://github.com/notadragon/gnu_gcc/commit/cd4981120f7a5e343ab146732e7d774e228a5eeb) | None found (searched 2026-09-26) | [gcc-503-lambda-contract-only-capture.md](gcc-503/gcc-503-lambda-contract-only-capture.md) |
| GCC-596 | A lambda in one member's precondition plus a bad result name in a member template's postcondition crash after the error | Fixed here | (not identified) | None found (searched 2026-09-30) | [gcc-596-member-lambda-pre-template-post-ice.md](gcc-596/gcc-596-member-lambda-pre-template-post-ice.md) |
| GCC-607 | With `-fcontracts-client-check=pre`/`all`, a constexpr call with a precondition is not a constant expression (non-constexpr caller-side wrapper) | Fixed here | [2bcd6b6193ba](https://github.com/notadragon/gnu_gcc/commit/2bcd6b6193ba24de51cd876776bb99ffb998d89f), [70ce4a4c4235](https://github.com/notadragon/gnu_gcc/commit/70ce4a4c4235c16d5f7ce47b1b3f033776068563) | None found (searched 2026-09-30) | [gcc-607-constexpr-caller-side-wrapper.md](gcc-607/gcc-607-constexpr-caller-side-wrapper.md) |
| GCC-608 | A defaulting redeclaration that adds a contract (`S::~S () pre (true) = default;`) crashes after its error | Fixed here | (not identified) | None found; likely a sibling of PR125403 | [gcc-608-defaulted-dtor-adds-contract.md](gcc-608/gcc-608-defaulted-dtor-adds-contract.md) |
| GCC-618 | A const local's constant-initialization attempt checks contracts: a precondition that is not constant, or false, in an initializer that is not constant anyway is a hard error instead of leaving the variable to run-time initialization | Fixed here | [d7ea932891a4](https://github.com/notadragon/gnu_gcc/commit/d7ea932891a4d361b6b96308354a8c8f9ddc7460) | None found (searched 2026-09-29) | [gcc-618-const-local-init-checks-contracts.md](gcc-618/gcc-618-const-local-init-checks-contracts.md) |
| GCC-620 | A contract violation found while constant-evaluating an array bound is diagnosed twice, byte-identical at the same location | Fixed here |  | None found (searched 2026-09-29) | [gcc-620-array-bound-contract-diagnosed-twice.md](gcc-620/gcc-620-array-bound-contract-diagnosed-twice.md) |
| GCC-630 | A friend function's contract predicate does not find members of the class it is declared in ("not declared in this scope"), whether the friend is defined in the class or outside it | Fixed here | [8a434a321f10](https://github.com/notadragon/gnu_gcc/commit/8a434a321f101059042dc0feabae722d0b18ebda) | None found (searched 2026-10-01) | [gcc-630-friend-contract-class-scope.md](gcc-630/gcc-630-friend-contract-class-scope.md) |
| GCC-632 | A variable declared in a statement expression in a lambda's precondition or postcondition ICEs in `gimplify_var_or_parm_decl` | Fixed here | [f3ba0e722bf2](https://github.com/notadragon/gnu_gcc/commit/f3ba0e722bf2d2b7a05201c7a63846ca730d64b6) | None found (searched 2026-10-01) | [gcc-632-stmt-expr-local-lambda-contract.md](gcc-632/gcc-632-stmt-expr-local-lambda-contract.md) |
| GCC-633 | A generic lambda whose precondition names one of its captures ICEs in `tsubst_expr` | Fixed here | [44c79798beb2](https://github.com/notadragon/gnu_gcc/commit/44c79798beb2eaa4f5087d2e837e0fb9580e3402) | None found (searched 2026-10-02) | [gcc-633-generic-lambda-pre-capture.md](gcc-633/gcc-633-generic-lambda-pre-capture.md) |
| GCC-634 | A fold over a captured pack in a lambda's precondition is rejected ("could not convert") and then ICEs; on this branch it ICEs | Fixed here | [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009) | None found (searched 2026-10-02) | [gcc-634-lambda-pre-fold-capture.md](gcc-634/gcc-634-lambda-pre-fold-capture.md) |
| GCC-641 | A lambda in a contract predicate may modify `*this` through a captured `this`, where `this` is a pointer to const | Fixed here | [bee70cf7e689](https://github.com/notadragon/gnu_gcc/commit/bee70cf7e689eccdcc184b10a0e9c75d4fb2d6d4) | None found (searched 2026-10-06) | [gcc-641-lambda-this-modifies-object.md](gcc-641/gcc-641-lambda-this-modifies-object.md) |
| GCC-644 | A local named in `typeid` of a polymorphic glvalue in a lambda with no capture-default is not captured and not diagnosed (a GCC 17 regression; not a contracts bug) | Open |  | None found (searched 2026-10-06) | [gcc-644-typeid-polymorphic-capture.md](gcc-644/gcc-644-typeid-polymorphic-capture.md) |
| GCC-646 | A function template declared in a precompiled header and instantiated after it loses its contract assertions (depends on the `.gch` build) | Fixed here | [7fc5019a3981](https://github.com/notadragon/gnu_gcc/commit/7fc5019a3981b2ffa279fe17dbb9eef98d2b1a94) | None found (searched 2026-10-06) | [gcc-646-pch-template-contracts.md](gcc-646/gcc-646-pch-template-contracts.md) |
| GCC-647 | A redeclaration after a precompiled header is checked as if the header's declarations had no contracts | Fixed here | [7fc5019a3981](https://github.com/notadragon/gnu_gcc/commit/7fc5019a3981b2ffa279fe17dbb9eef98d2b1a94) | None found (searched 2026-10-06) | [gcc-647-pch-redecl-contracts.md](gcc-647/gcc-647-pch-redecl-contracts.md) |
| GCC-648 | A parameter pack used only in a lambda's contract assertion is implicitly captured without a diagnostic | Fixed here | [60072d1efe38](https://github.com/notadragon/gnu_gcc/commit/60072d1efe38f36174f3e78ac153adcb3022f3be) | None found (searched 2026-10-06) | [gcc-648-contract-only-capture-pack.md](gcc-648/gcc-648-contract-only-capture-pack.md) |
| GCC-649 | A constant read in a lambda's contract assertion is diagnosed as "not implicitly captured by a contract assertion" | Fixed here | [60072d1efe38](https://github.com/notadragon/gnu_gcc/commit/60072d1efe38f36174f3e78ac153adcb3022f3be) | None found (searched 2026-10-06) | [gcc-649-contract-only-capture-constant.md](gcc-649/gcc-649-contract-only-capture-constant.md) |
| GCC-650 | A parameter named only in a discarded member access in a postcondition is required to be const | Fixed here | [16b8ae6840ca](https://github.com/notadragon/gnu_gcc/commit/16b8ae6840cacd49f0592ad23cbc710d951effad) | [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) (related) | [gcc-650-postcondition-discarded-member-access.md](gcc-650/gcc-650-postcondition-discarded-member-access.md) |
| GCC-651 | An unexpanded parameter pack in a contract predicate is not diagnosed in the template | Fixed here | [a6703a3b216b](https://github.com/notadragon/gnu_gcc/commit/a6703a3b216b30803d15fbbeb2b3dce4ba748e45) | None found (searched 2026-10-06) | [gcc-651-contract-unexpanded-pack.md](gcc-651/gcc-651-contract-unexpanded-pack.md) |
| GCC-652 | A statement expression's variable in a generic lambda's precondition is rejected when the operator() is instantiated | Fixed here | [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009) | None found (searched 2026-10-06) | [gcc-652-stmt-expr-local-generic-lambda-contract.md](gcc-652/gcc-652-stmt-expr-local-generic-lambda-contract.md) |
| GCC-656 | A lambda in a function template's precondition cannot capture the template's parameter pack | Fixed here | [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009) | None found (searched 2026-10-06) | [gcc-656-predicate-lambda-pack-capture.md](gcc-656/gcc-656-predicate-lambda-pack-capture.md) |
| GCC-663 | A function template's parameter named only as the object of a discarded member access in a postcondition is required to be const on the definition | Fixed here | [16b8ae6840ca](https://github.com/notadragon/gnu_gcc/commit/16b8ae6840cacd49f0592ad23cbc710d951effad) | None found (searched 2026-10-07); related [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) | [gcc-663-postcondition-discarded-member-access-template.md](gcc-663/gcc-663-postcondition-discarded-member-access-template.md) |
| GCC-682 | `noexcept` of an initializer list ignores a potentially-throwing default constructor run by its array filler (`noexcept(A{})` with `struct A { S s[2]; }`; `A{{}}` and `S[2]{}` a GCC 14 regression) | Fixed here | [270b8e0a50bd](https://github.com/notadragon/gnu_gcc/commit/270b8e0a50bd5c80394559ccf382909f2dbeb2fd) | None found (searched 2026-10-09) | [gcc-682-noexcept-array-filler.md](gcc-682/gcc-682-noexcept-array-filler.md) |
| GCC-683 | `noexcept` of a delete-expression whose operand is a null pointer constant ignores a potentially-throwing destructor | Open |  | [PR65174](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65174) | [gcc-683-noexcept-delete-null.md](gcc-683/gcc-683-noexcept-delete-null.md) |
| GCC-689 | A potentially-throwing contract predicate checked under quick_enforce draws a bogus `-Wmaybe-uninitialized` warning about a compiler temporary | Fixed here | [76208014c18f](https://github.com/notadragon/gnu_gcc/commit/76208014c18f720e81e3c9a908704a916cc0b2de) | [PR125163](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125163) (comment 2 only; no PR of its own, searched 2026-10-10) | [gcc-689-quick-enforce-throwing-predicate-uninit.md](gcc-689/gcc-689-quick-enforce-throwing-predicate-uninit.md) |
| GCC-706 | A contract predicate spanning a line of only blanks ICEs in `get_source_text_between` | Fixed here | [9562c7a688cf](https://github.com/notadragon/gnu_gcc/commit/9562c7a688cf8814887db4c491e7363a54fd0c3b) | [PR127857](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127857) | [gcc-706-contract-comment-blank-line.md](gcc-706/gcc-706-contract-comment-blank-line.md) |
| GCC-707 | A contract predicate that begins or ends inside a macro expansion gets a wrong comment, which can include unrelated source | Fixed here | [d8d5a88ff486](https://github.com/notadragon/gnu_gcc/commit/d8d5a88ff4868d13b5ef3ab8565fc43bcfb70e05) | None found (searched 2026-10-12) | [gcc-707-contract-comment-macro.md](gcc-707/gcc-707-contract-comment-macro.md) |
| GCC-734 | `-fsanitize=vptr` does not check the reference form of a `static_cast` downcast (`static_cast<D &> (b)`, `static_cast<D &&> (b)`) | Fixed here | [e57470f55d57](https://github.com/notadragon/gnu_gcc/commit/e57470f55d572df47b16d995e0224df2be6d389f) | [PR116102](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116102) | [gcc-734-vptr-reference-downcast.md](gcc-734/gcc-734-vptr-reference-downcast.md) |
| GCC-773 | `-fsanitize=vptr` does not check a member of a polymorphic base class reached through a glvalue of the wrong derived type (`reinterpret_cast<B &> (a).x`, `x` a member of a base shared by `A` and `B`) | Open |  | None found (searched 2026-10-13) -- filing owed | [gcc-773-vptr-base-member.md](gcc-773/gcc-773-vptr-base-member.md) |
| GCC-504 | A hidden friend with a precondition in a class template: ICE, or a dependent type left in the predicate | Fixed here | [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009) | None found (searched 2026-09-26) | [gcc-504-hidden-friend-template-pre.md](gcc-504/gcc-504-hidden-friend-template-pre.md) |
| GCC-506 | A lambda whose precondition names a member, in a default member initializer of a class template, is rejected (ICE on this branch) | Fixed here | [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009) | [PR127287](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127287) * (same defect, member-through-`this` face) + comment pending | [gcc-506-nsdmi-lambda-contract-this.md](gcc-506/gcc-506-nsdmi-lambda-contract-this.md) |
| GCC-507 | Implicit member access inside a lambda in a constructor precondition is accepted | Fixed here | [bff299b3f396](https://github.com/notadragon/gnu_gcc/commit/bff299b3f396e92d15db7b7f99a48103d042753b) | None found (searched 2026-09-26) | [gcc-507-ctor-pre-member-in-lambda.md](gcc-507/gcc-507-ctor-pre-member-in-lambda.md) |
| GCC-510 | A precompiled header's contract evaluation semantic silently overrides the TU's | Fixed here | [83c13da929a0](https://github.com/notadragon/gnu_gcc/commit/83c13da929a08467390e8171ae516e9b34b81df8) | None found (searched 2026-09-26) | [gcc-510-pch-contract-semantic-overrides-tu.md](gcc-510/gcc-510-pch-contract-semantic-overrides-tu.md) |
| GCC-517 | A postcondition result name that conflicts with a parameter is diagnosed only in a checked definition | Fixed here | [b6eece227f40](https://github.com/notadragon/gnu_gcc/commit/b6eece227f40af201fe7903ceb32a6fc8cd46985) | None found (searched 2026-09-26) | [gcc-517-result-name-shadows-parameter.md](gcc-517/gcc-517-result-name-shadows-parameter.md) |
| GCC-518 | A redeclaration that adds or drops a postcondition's result-name-introducer is accepted | Fixed here | [b6eece227f40](https://github.com/notadragon/gnu_gcc/commit/b6eece227f40af201fe7903ceb32a6fc8cd46985) | None found (searched 2026-09-26) | [gcc-518-redecl-result-name-presence.md](gcc-518/gcc-518-redecl-result-name-presence.md) |
| GCC-521 | Redeclaration contract predicates are compared after constant folding | Fixed here | [b6eece227f40](https://github.com/notadragon/gnu_gcc/commit/b6eece227f40af201fe7903ceb32a6fc8cd46985) | None found (searched 2026-09-26) | [gcc-521-redecl-condition-folded.md](gcc-521/gcc-521-redecl-condition-folded.md) |
| GCC-523 | Contract wrappers drop the callee's `noreturn` and `nodiscard`, and diagnostics name the wrapper | Fixed here | [2bcd6b6193ba](https://github.com/notadragon/gnu_gcc/commit/2bcd6b6193ba24de51cd876776bb99ffb998d89f) | None found (searched 2026-09-26) | [gcc-523-contract-wrapper-drops-attributes.md](gcc-523/gcc-523-contract-wrapper-drops-attributes.md) |
| GCC-543 | A lambda in a declaration's contract, then any default argument -> ICE | Fixed here | [de4538e04341](https://github.com/notadragon/gnu_gcc/commit/de4538e0434118f629564a09cc2157e27abc81ec) | [PR127428](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127428) + comment pending | [gcc-543-contract-lambda-then-default-arg-ice.md](gcc-543/gcc-543-contract-lambda-then-default-arg-ice.md) |
| GCC-544 | A lambda in the in-class contract of a templated member function -> ICE | Fixed here | [de4538e04341](https://github.com/notadragon/gnu_gcc/commit/de4538e0434118f629564a09cc2157e27abc81ec) | None found (searched 2026-09-26) | [gcc-544-template-decl-contract-lambda-ice.md](gcc-544/gcc-544-template-decl-contract-lambda-ice.md) |
| GCC-551 | The contract on a block-scope first declaration is dropped (or reported as "added") | Fixed here | [1b63e74eb262](https://github.com/notadragon/gnu_gcc/commit/1b63e74eb26215f7738bfbf07d2a8d8de7b3818e) | None found (searched 2026-09-26) | [gcc-551-block-scope-first-decl-contract.md](gcc-551/gcc-551-block-scope-first-decl-contract.md) |
| GCC-612 | A templated function declared with a contract whose lambda captures a parameter, defined without repeating it -> ICE on instantiation | Fixed here | [934ddf891091](https://github.com/notadragon/gnu_gcc/commit/934ddf891091ae4a327615927d36c697515632be) | None found (searched 2026-09-26) | [gcc-612-redecl-omit-contract-lambda-ice.md](gcc-612/gcc-612-redecl-omit-contract-lambda-ice.md) |
| GCC-552 | An explicit specialization declared with a contract and defined without one loses the contract | Fixed here | [937b5a592599](https://github.com/notadragon/gnu_gcc/commit/937b5a592599c4e5359d8e8477b31a2231bbf7f0) | None found (searched 2026-09-26) | [gcc-552-explicit-spec-contract-dropped.md](gcc-552/gcc-552-explicit-spec-contract-dropped.md) |
| GCC-553 | Templates exported from a module lose their contracts | Fixed here | [de7b14fe9a9c](https://github.com/notadragon/gnu_gcc/commit/de7b14fe9a9c34d5cc85aa40270d8dbb4a83bf46) | None found (searched 2026-09-26) | [gcc-553-module-template-contract-dropped.md](gcc-553/gcc-553-module-template-contract-dropped.md) |
| GCC-569 | A null object pointer through `->*` (pointer to data member) is not checked | Fixed here | [1cbb53345583](https://github.com/notadragon/gnu_gcc/commit/1cbb53345583d657a7fd5beee1bfa2f176c712d0) | None found (searched 2026-09-26) | [gcc-569-ptrmem-null-object-not-checked.md](gcc-569/gcc-569-ptrmem-null-object-not-checked.md) |
| GCC-574 | A constructor-precondition / destructor-postcondition member rule rejects unevaluated uses (`sizeof(b)`) and accepts the qualified form `pre(C::b)` | Fixed here | [8bbcde847947](https://github.com/notadragon/gnu_gcc/commit/8bbcde847947d826be592cbc520a2289b9b6c812) | None found (searched 2026-09-26) | [gcc-574-ctor-pre-member-unevaluated-and-qualified.md](gcc-574/gcc-574-ctor-pre-member-unevaluated-and-qualified.md) |
| GCC-606 | An explicit by-copy lambda capture in a contract predicate gets the constified type, so a mutable lambda cannot modify its copy | Fixed here | [ebd73576d396](https://github.com/notadragon/gnu_gcc/commit/ebd73576d396ea81dc1e09e6b01d3fb2f7c76ca9) | None found (searched 2026-09-26) | [gcc-606-contract-explicit-copy-capture-const.md](gcc-606/gcc-606-contract-explicit-copy-capture-const.md) |
| GCC-610 | A lambda capture named in a contract crashes the compiler under -fcontract-checks-outlined | Fixed here | [934ddf891091](https://github.com/notadragon/gnu_gcc/commit/934ddf891091ae4a327615927d36c697515632be) | None found (searched 2026-09-26) | [gcc-610-outlined-lambda-capture-proxy-ice.md](gcc-610/gcc-610-outlined-lambda-capture-proxy-ice.md) |
| GCC-710 | ICE (checking) in `cxx_eval_constant_expression`, constexpr.cc:11356, on a constexpr use of a heap object after its destructor ran | Fixed here | [10ef2099e1a5](https://github.com/notadragon/gnu_gcc/commit/10ef2099e1a52c3af7d7d10bc2abbd59887182a8) | None found as an ICE (searched 2026-10-12) | [gcc-710-ice-heap-object-after-dtor.md](gcc-710/gcc-710-ice-heap-object-after-dtor.md) |
| GCC-711 | ICE in `cxx_eval_constant_expression`, constexpr.cc:10816, diagnosing a `static_cast` from `void*` whose operand is `a + N` | Open |  | [PR125199](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125199) | [gcc-711-ice-void-cast-pointer-plus.md](gcc-711/gcc-711-ice-void-cast-pointer-plus.md) |
| GCC-712 | ICE in `synthesized_method_walk`, method.cc:3031, explaining why an implicit destructor with a virtual base is not constexpr | Open |  | [PR123681](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123681) | [gcc-712-ice-synthesized-method-walk.md](gcc-712/gcc-712-ice-synthesized-method-walk.md) |
| GCC-713 | A reference `reinterpret_cast` is accepted in a constant expression | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-713-constexpr-reference-reinterpret-cast.md](gcc-713/gcc-713-constexpr-reference-reinterpret-cast.md) |
| GCC-714 | Double destruction of an automatic object is accepted in constant evaluation when the destructor does not touch the object | Open |  | [PR102284](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102284) (comment 2 is this case) | [gcc-714-constexpr-double-destruction-automatic.md](gcc-714/gcc-714-constexpr-double-destruction-automatic.md) |
| GCC-715 | rejects-valid: a pointer to member converted to a base of the containing class is not a constant expression when applied | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-715-rejects-valid-ptrmem-cast-to-base.md](gcc-715/gcc-715-rejects-valid-ptrmem-cast-to-base.md) |
| GCC-716 | `static_cast` of an out-of-range floating value to an unfixed enumeration is rejected with an unhelpful diagnostic | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-716-enum-floating-cast-diagnostic.md](gcc-716/gcc-716-enum-floating-cast-diagnostic.md) |
| GCC-717 | UBSan `vptr` reports a defined `static_cast` downcast performed during construction of the derived object | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-717-ubsan-vptr-downcast-during-construction.md](gcc-717/gcc-717-ubsan-vptr-downcast-during-construction.md) |
| GCC-739 | Forming a pointer beyond one past the end of an array is accepted in constant evaluation (only subscripts and reads are rejected) | Open |  | [PR70151](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70151), [PR111242](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111242) (heap); PR82876 (overflow) | [gcc-739-constexpr-oob-pointer-formation.md](gcc-739/gcc-739-constexpr-oob-pointer-formation.md) |
| GCC-740 | Subtracting pointers to different members (or rows) of one object is accepted in constant evaluation | Open |  | [PR126060](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126060) (rows; the members form is not shown there) | [gcc-740-constexpr-sub-same-object.md](gcc-740/gcc-740-constexpr-sub-same-object.md) |
| GCC-741 | `static_cast` of an out-of-range integral value to an unfixed enumeration is accepted in constant evaluation | Open |  | [PR95701](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95701) (DR1766; PR117963, PR106294 duplicates; PR126353 the same, not yet marked) | [gcc-741-constexpr-enum-out-of-range.md](gcc-741/gcc-741-constexpr-enum-out-of-range.md) |
| GCC-742 | `static_cast` of a pointer to member to a base class that does not contain the member is accepted in constant evaluation | Open |  | [PR109243](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109243) (PR121579, PR126639 duplicates) | [gcc-742-constexpr-ptrmem-cast-unrelated-base.md](gcc-742/gcc-742-constexpr-ptrmem-cast-unrelated-base.md) |
| GCC-743 | Applying a pointer to member to an object that does not contain the member is accepted in constant evaluation when no data is read | Open |  | [PR121250](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121250), [PR105277](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105277) | [gcc-743-constexpr-ptrmem-object-lacks-member.md](gcc-743/gcc-743-constexpr-ptrmem-object-lacks-member.md) |
| GCC-744 | Reading a volatile object through a non-volatile glvalue is accepted in constant evaluation | Open |  | [PR126359](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126359) | [gcc-744-constexpr-volatile-through-nonvolatile.md](gcc-744/gcc-744-constexpr-volatile-through-nonvolatile.md) |
| GCC-745 | Creating an object in the storage of a const complete object is accepted in constant evaluation | Open |  | [PR119304](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119304) | [gcc-745-constexpr-create-in-const-object.md](gcc-745/gcc-745-constexpr-create-in-const-object.md) |
| GCC-746 | A reference used in its own initializer is accepted in constant evaluation | Open |  | [PR119339](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119339) | [gcc-746-constexpr-reference-self-init.md](gcc-746/gcc-746-constexpr-reference-self-init.md) |
| GCC-747 | `dynamic_cast` during construction through a not-yet-constructed path is accepted in constant evaluation | Open |  | [PR93096](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93096) (the standard's own example) | [gcc-747-constexpr-cdtor-dynamic-cast.md](gcc-747/gcc-747-constexpr-cdtor-dynamic-cast.md) |
| GCC-748 | An unsequenced modification and read of a scalar is accepted in constant evaluation | Open |  | [PR87275](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87275) (PR116695 duplicate) | [gcc-748-constexpr-unsequenced-modification.md](gcc-748/gcc-748-constexpr-unsequenced-modification.md) |
| GCC-749 | Indirection through a null pointer that is not followed by a read is accepted in constant evaluation | Open |  | [PR115592](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115592) (CWG2823), [PR126360](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126360), [PR113047](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113047) | [gcc-749-constexpr-null-indirection-no-read.md](gcc-749/gcc-749-constexpr-null-indirection-no-read.md) |
| GCC-750 | Conversion to a virtual base of a named object after its destructor is accepted in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-750-constexpr-vbase-conversion-destroyed-named.md](gcc-750/gcc-750-constexpr-vbase-conversion-destroyed-named.md) |
| GCC-751 | Member function call on an object outside its lifetime is not rejected in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-751-constexpr-member-call-outside-lifetime.md](gcc-751/gcc-751-constexpr-member-call-outside-lifetime.md) |
| GCC-752 | dynamic_cast and typeid of an object outside its lifetime accepted in constant evaluation when the result is not used | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-752-constexpr-dynamic-cast-typeid-outside-lifetime.md](gcc-752/gcc-752-constexpr-dynamic-cast-typeid-outside-lifetime.md) |
| GCC-753 | delete[] through a pointer to base not rejected in constant evaluation (same-size or one-element arrays) | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-753-constexpr-array-delete-via-base.md](gcc-753/gcc-753-constexpr-array-delete-via-base.md) |
| GCC-754 | Modification of an object created by new const T is not rejected in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-754-constexpr-modify-new-const.md](gcc-754/gcc-754-constexpr-modify-new-const.md) |
| GCC-755 | Referring to a member in a constructor's function-try-block handler is not rejected in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-755-constexpr-function-try-block-handler-member.md](gcc-755/gcc-755-constexpr-function-try-block-handler-member.md) |
| GCC-756 | &(s.*pm) with a null pointer to data member accepted in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-756-constexpr-null-data-member-pointer-address.md](gcc-756/gcc-756-constexpr-null-data-member-pointer-address.md) |
| GCC-757 | dynamic_cast downcast of a past-the-end pointer accepted in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-757-constexpr-dynamic-cast-past-end-downcast.md](gcc-757/gcc-757-constexpr-dynamic-cast-past-end-downcast.md) |
| GCC-758 | Virtual call in a base mem-initializer not rejected in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-758-constexpr-virtual-call-base-mem-init.md](gcc-758/gcc-758-constexpr-virtual-call-base-mem-init.md) |
| GCC-759 | Conversion of this to a virtual base whose deriving base has not started construction accepted in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-759-constexpr-cdtor-convert-typeid-virtual.md](gcc-759/gcc-759-constexpr-cdtor-convert-typeid-virtual.md) |
| GCC-760 | Direct call to a pure virtual function with a constexpr definition from its class's constructor is accepted in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-760-constexpr-pure-virtual-direct-call.md](gcc-760/gcc-760-constexpr-pure-virtual-direct-call.md) |
| GCC-761 | Using a reference whose referent's storage has been deallocated is not rejected in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-761-constexpr-reference-to-freed-storage.md](gcc-761/gcc-761-constexpr-reference-to-freed-storage.md) |
| GCC-762 | Overlapping built-in assignment between union members not rejected in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-762-constexpr-union-overlap-assignment.md](gcc-762/gcc-762-constexpr-union-overlap-assignment.md) |
| GCC-763 | Binding a reference through a past-the-end pointer accepted in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-763-constexpr-reference-through-invalid-pointer.md](gcc-763/gcc-763-constexpr-reference-through-invalid-pointer.md) |
| GCC-764 | `delete` of storage obtained from `std::allocator::allocate` is accepted in constant evaluation | Open |  | None found (searched 2026-10-12) -- filing owed | [gcc-764-constexpr-delete-allocator-storage.md](gcc-764/gcc-764-constexpr-delete-allocator-storage.md) |
| GCC-779 | A placement new of an empty class at a null pointer is accepted in constant evaluation | Open |  | None found (searched 2026-10-13) -- filing owed | [gcc-779-constexpr-placement-new-null-empty.md](gcc-779/gcc-779-constexpr-placement-new-null-empty.md) |
| GCC-784 | A member function call, or a `dynamic_cast` of `this`, in a base mem-initializer is accepted in constant evaluation | Open |  | [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (the call) | [gcc-784-constexpr-call-in-base-mem-init.md](gcc-784/gcc-784-constexpr-call-in-base-mem-init.md) |
| GCC-785 | `&*(a + N)`, an indirection through a past-the-end pointer, is accepted in constant evaluation | Open |  | None found (searched 2026-10-13) -- filing owed | [gcc-785-constexpr-address-past-end-indirection.md](gcc-785/gcc-785-constexpr-address-past-end-indirection.md) |
| GCC-786 | Binding a reference through a pointer to an automatic object whose scope has ended is accepted in constant evaluation | Open |  | None found (searched 2026-10-13) -- filing owed | [gcc-786-constexpr-reference-through-dangling-pointer.md](gcc-786/gcc-786-constexpr-reference-through-dangling-pointer.md) |
| GCC-831 | An empty class stored to, or assigned from, a past-the-end array element is accepted in constant evaluation | Open |  | None found (searched 2026-10-15) -- filing owed | [gcc-831-constexpr-empty-store-past-end.md](gcc-831/gcc-831-constexpr-empty-store-past-end.md) |
| GCC-854 | `-fsanitize=null` does not check the object pointer of a member call returning a class with a non-trivial copy constructor or destructor by value (an `AGGR_INIT_EXPR`) | Open |  | None found (searched 2026-10-16) -- filing owed | [gcc-854-null-member-call-class-return.md](gcc-854/gcc-854-null-member-call-class-return.md) |
| GCC-871 | `-fsanitize=null` reports a downcast of `*p` to a reference, `p` a null pointer, twice when the base is at a nonzero offset in the derived class (the conversion's null arm instrumented by `build_static_cast_1` and again by `cp_genericize_r`) | Open |  | None found (searched 2026-10-17) -- filing owed | [gcc-871-null-downcast-ref-twice.md](gcc-871/gcc-871-null-downcast-ref-twice.md) |
| GCC-877 | `-fsanitize=alignment` does not check the pointer of a placement new that stores nothing (an empty class, default-initialization): only the stores of the initialization are checked | Open |  | None found (searched 2026-10-18) -- filing owed | [gcc-877-placement-new-align-nostore.md](gcc-877/gcc-877-placement-new-align-nostore.md) |
| GCC-896 | An inline function (or template instantiation, or C99 inline definition) whose contract assertion one TU ignores is found nothrow, const and store-free from that TU's body, and its callers there are compiled so; when the linker keeps another TU's copy, which evaluates the assertion with a throwing or side-effecting violation handler, the callers' `catch` and cleanups are skipped (`std::terminate`) or a store by the handler is not reloaded | Fixed here | [76208014c18f](https://github.com/notadragon/gnu_gcc/commit/76208014c18f720e81e3c9a908704a916cc0b2de) [5fccf4269c80](https://github.com/notadragon/gnu_gcc/commit/5fccf4269c807dec5d3b87d9bfe87c9cbf64fa41) | None found (searched 2026-10-21; general case PR121936) -- filing owed | [gcc-896-ignore-copy-facts-cross-tu.md](gcc-896/gcc-896-ignore-copy-facts-cross-tu.md) |
| GCC-897 | `-fipa-pta` models a direct call of a weak function (or an exported one under `-fPIC` semantic interposition, or a `noipa` one) by its body, so a caller's object passed to it is found not to escape although the definition linked in its place may let it escape | Fixed here | [30ec133d5d7e](https://github.com/notadragon/gnu_gcc/commit/30ec133d5d7e731cbce3ed506cf8cd8cfb03fdf9) | None found (searched 2026-10-21) -- filing owed | [gcc-897-ipa-pta-weak-callee.md](gcc-897/gcc-897-ipa-pta-weak-callee.md) |
| GCC-910 | A VLA captured by reference in a lambda in a function template ICEs in `set_contract_capture_flag` at instantiation in a C++26 dialect, with no contract in the program (GCC-495's defect from valid code) | Fixed here | [cd4981120f7a](https://github.com/notadragon/gnu_gcc/commit/cd4981120f7a5e343ab146732e7d774e228a5eeb) | [PR127911](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127911) | [gcc-910-vla-capture-template-ice.md](gcc-910/gcc-910-vla-capture-template-ice.md) |
| GCC-911 | `-g` changes the order of the caller-side contract wrappers (and, optimizing, of their violation objects), so `-fcompare-debug` fails | Fixed here | [2bcd6b6193ba](https://github.com/notadragon/gnu_gcc/commit/2bcd6b6193ba24de51cd876776bb99ffb998d89f) | [PR128021](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128021) | [gcc-911-wrapper-order-compare-debug.md](gcc-911/gcc-911-wrapper-order-compare-debug.md) |

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
