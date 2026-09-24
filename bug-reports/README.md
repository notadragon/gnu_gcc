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
afterwards, which is the common case here: of the 33 PRs linked below, 21 are
other people's.  On our own PR we can comment freely and are expected to
answer questions; on someone else's the useful contribution is usually a
measurement or a confirmation, not a retelling.

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
| GCC-1 | Contract on a function with a variadic parameter pack ICEs or misattributes a diagnostic | Fixed here | [0144d0420135](https://github.com/notadragon/gnu_gcc/commit/0144d0420135b44c8ea4376230e38d9be6bfa0e8) | [PR124395](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124395) (+5 duplicates -- see details) | [gcc-01-contract-pack-ice.md](gcc-01/gcc-01-contract-pack-ice.md) |
| GCC-2 | Constant evaluator does not diagnose forming an address into an object before its constructor begins, for a virtual base | Open |  | [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (partial -- see details) | [gcc-02-constexpr-vbase-before-ctor.md](gcc-02/gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-10 | Contract predicate on a lambda capturing `this` reads the closure object as the enclosing class | Fixed here | [984f2e891022](https://github.com/notadragon/gnu_gcc/commit/984f2e8910220b14872e6c27cfae9b32d04c7b46) | [PR127283](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127283) * | [gcc-10-lambda-this-capture-in-contract.md](gcc-10/gcc-10-lambda-this-capture-in-contract.md) |
| GCC-11 | Lambda in a non-member postcondition with a result name fails to parse | Fixed here | [2c3b9f330234](https://github.com/notadragon/gnu_gcc/commit/2c3b9f330234300918db6338363eee65c90b72ab) | [PR127284](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127284) * | [gcc-11-lambda-in-postcondition-result-name.md](gcc-11/gcc-11-lambda-in-postcondition-result-name.md) |
| GCC-12 | A postcondition's result binding denotes two different objects within one predicate evaluation | Fixed here | [dab8f9149702](https://github.com/notadragon/gnu_gcc/commit/dab8f9149702172cd7e6c00bea1b56b54ddb9385) | [PR112794](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112794) | [gcc-12-result-binding-denotes-two-objects.md](gcc-12/gcc-12-result-binding-denotes-two-objects.md) |
| GCC-13 | Lambda's own contract specifiers are never substituted when it's instantiated | Fixed here | [61834e3127f2](https://github.com/notadragon/gnu_gcc/commit/61834e3127f272612a21ba9c063d6de2f5154805) | [PR127287](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127287) * | [gcc-13-precondition-in-lambda-nested-in-generic-lambda.md](gcc-13/gcc-13-precondition-in-lambda-nested-in-generic-lambda.md) |
| GCC-14 | Garbage source location on a contract-capture diagnostic note | Fixed here | [9ddaf249bb93](https://github.com/notadragon/gnu_gcc/commit/9ddaf249bb930bf498ab0f2e2d0ee7d73fff96ae) | [PR126041](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126041) | [gcc-14-contract-capture-note-garbage-location.md](gcc-14/gcc-14-contract-capture-note-garbage-location.md) |
| GCC-15 | Outlined contract checks lose by-value parameter/result mutations | Fixed here | [4b6812f3542a](https://github.com/notadragon/gnu_gcc/commit/4b6812f3542ae96084f95c985e64e03ab61502f3) | [PR127289](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127289) * | [gcc-15-outlined-checks-lose-by-value-mutations.md](gcc-15/gcc-15-outlined-checks-lose-by-value-mutations.md) |
| GCC-16 | Contract on a capturing lambda inside an instantiated template segfaults | Fixed here | [10de22c1d3c9](https://github.com/notadragon/gnu_gcc/commit/10de22c1d3c9a3cf427c8abee365d4bb86b319dc), [61834e3127f2](https://github.com/notadragon/gnu_gcc/commit/61834e3127f272612a21ba9c063d6de2f5154805) | [PR126038](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126038) | [gcc-16-lambda-capture-contract-in-template.md](gcc-16/gcc-16-lambda-capture-contract-in-template.md) |
| GCC-17 | `this` accepted in the trailing return type of an explicit-object member function | Open |  | [PR127290](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127290) * | [gcc-17-this-in-xobj-declaration.md](gcc-17/gcc-17-this-in-xobj-declaration.md) |
| GCC-18 | A predicate lambda naming a namespace-scope variable does not constify it | Fixed here | [83c061ffdbd0](https://github.com/notadragon/gnu_gcc/commit/83c061ffdbd036002e0314b070b1006606d8f2c1) | [PR127293](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127293) * | [gcc-18-lambda-in-predicate-global-not-constified.md](gcc-18/gcc-18-lambda-in-predicate-global-not-constified.md) |
| GCC-19 | A discarded comma operand in a postcondition is wrongly treated as odr-using a by-value parameter | Fixed here | [5f05302dac69](https://github.com/notadragon/gnu_gcc/commit/5f05302dac6938b244dd7eec247ec22819be29dc) | [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) | [gcc-19-postcondition-comma-odr-use.md](gcc-19/gcc-19-postcondition-comma-odr-use.md) |
| GCC-21 | `-std=c++26` alone does not link the contract violation handler | Fixed here |  | [PR126158](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126158) | [gcc-21-cpp26-missing-experimental-link.md](gcc-21/gcc-21-cpp26-missing-experimental-link.md) |
| GCC-22 | A parameter pack of reference type in a contract is rejected outright | Fixed here | [dbcf8d7d5a27](https://github.com/notadragon/gnu_gcc/commit/dbcf8d7d5a27b4eca5663cf1e35357ed4d3963f9) | [PR126878](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126878), [PR126039](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126039) | [gcc-22-pack-reference-constification.md](gcc-22/gcc-22-pack-reference-constification.md) |
| GCC-23 | [dcl.contract.func]/6 not applied to deleted or first-declaration-defaulted functions | Fixed here | [48ea4aaf8bde](https://github.com/notadragon/gnu_gcc/commit/48ea4aaf8bde55355a752d94fc5496bd64ea88fb) | [PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486), [PR125403](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125403) | [gcc-23-contract-on-deleted-or-defaulted.md](gcc-23/gcc-23-contract-on-deleted-or-defaulted.md) |
| GCC-24 | A noexcept function with a contract segfaults the compiler under `-fno-enforce-eh-specs` | Fixed here | [0a0b0dc72ce0](https://github.com/notadragon/gnu_gcc/commit/0a0b0dc72ce0e3bcb03edc4903c5a0ca7f6bd985) | [PR127173](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127173) | [gcc-24-noexcept-body-wrapper-assumption.md](gcc-24/gcc-24-noexcept-body-wrapper-assumption.md) |
| GCC-25 | A postcondition's result-name-introducer does not accept `identifier attribute-specifier-seq :` | Fixed here | [9d52850c0163](https://github.com/notadragon/gnu_gcc/commit/9d52850c0163616b6da4c0d61b138cc0e45183fa) | [PR125725](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125725) | [gcc-25-postcondition-result-name-attribute.md](gcc-25/gcc-25-postcondition-result-name-attribute.md) |
| GCC-26 | A redeclaration whose parameter type is dependent escapes the postcondition const rule | Fixed here | [0144d0420135](https://github.com/notadragon/gnu_gcc/commit/0144d0420135b44c8ea4376230e38d9be6bfa0e8) | [PR127196](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127196) | [gcc-26-postcondition-redecl-dependent-param.md](gcc-26/gcc-26-postcondition-redecl-dependent-param.md) |
| GCC-27 | A friend declaration's contract is never checked against an earlier declaration of the same function (two shapes; the two-friend shape is fixed here, the namespace-scope-friend shape is not) | Open |  | [PR127291](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127291) * | [gcc-27-deferred-friend-contract-mismatch.md](gcc-27/gcc-27-deferred-friend-contract-mismatch.md) |
| GCC-28 | A member named unqualified in an explicit-object member function's contract is diagnosed with a constructor/destructor message | Fixed here | [4be9c83a7526](https://github.com/notadragon/gnu_gcc/commit/4be9c83a75265aae971a8ba3ee023bae5031d265) | [PR127294](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127294) * | [gcc-28-xobj-member-in-predicate-ctor-message.md](gcc-28/gcc-28-xobj-member-in-predicate-ctor-message.md) |
| GCC-30 | A coroutine's postcondition may odr-use a `const` by-value parameter | Fixed here | [03d6f29aae4b](https://github.com/notadragon/gnu_gcc/commit/03d6f29aae4bec2cb9f74fab2ce69973f98e66fa) | [PR127296](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127296) * | [gcc-30-coroutine-postcondition-const-param.md](gcc-30/gcc-30-coroutine-postcondition-const-param.md) |
| GCC-31 | A contract-assertion scope is not capture-transparent: a lambda in a predicate cannot capture the enclosing parameters, and once it can, a nested `contract_assert` naming a capture ICEs | Fixed here | [d9ac9d85a6b9](https://github.com/notadragon/gnu_gcc/commit/d9ac9d85a6b92adf08ecfa5506a2f2716c113ea9) | [PR117435](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117435) | [gcc-31-lambda-capture-in-contract-predicate.md](gcc-31/gcc-31-lambda-capture-in-contract-predicate.md) |
| GCC-33 | A class-type result binding passed to a function taking a reference ICEs at codegen | Fixed here | [dab8f9149702](https://github.com/notadragon/gnu_gcc/commit/dab8f9149702172cd7e6c00bea1b56b54ddb9385) | [PR125574](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125574) | [gcc-33-class-result-binding-by-reference.md](gcc-33/gcc-33-class-result-binding-by-reference.md) |
| GCC-36 | A non-template's postcondition const-parameter diagnostic is emitted twice, byte-identical at the same location | Fixed here | [5f05302dac69](https://github.com/notadragon/gnu_gcc/commit/5f05302dac6938b244dd7eec247ec22819be29dc) | [PR127297](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127297) * | [gcc-36-postcondition-const-rule-over-reported.md](gcc-36/gcc-36-postcondition-const-rule-over-reported.md) |
| GCC-37 | A `std::source_location` that is not a class (aliased away, or a union) ICEs the contract machinery | Fixed here | [77aaf753f615](https://github.com/notadragon/gnu_gcc/commit/77aaf753f6151e942d6788dc557e1315b8d0d653) | [PR127255](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127255) | [gcc-37-source-location-alias-ice.md](gcc-37/gcc-37-source-location-alias-ice.md) |
| GCC-38 | An unrelated precondition gives `std::source_location` internal linkage when the contract is seen before the header | Fixed here | [77aaf753f615](https://github.com/notadragon/gnu_gcc/commit/77aaf753f6151e942d6788dc557e1315b8d0d653) | [PR127251](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127251) | [gcc-38-source-location-linkage.md](gcc-38/gcc-38-source-location-linkage.md) |
| GCC-39 | Explicit instantiation of a variadic template rejects a cv-qualified pack argument (not a contracts bug) | Open |  | [PR126797](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126797) | [gcc-39-explicit-inst-pack-cv.md](gcc-39/gcc-39-explicit-inst-pack-cv.md) |
| GCC-42 | A contract predicate that modifies the enclosing constant evaluation is rejected as `contract condition is not constant`, rejecting a well-formed program | Fixed here | [53dcb8cbbda1](https://github.com/notadragon/gnu_gcc/commit/53dcb8cbbda155f40d9acc2bc702b0fbaba92015) | None found (searched 2026-09-11) | [gcc-42-contract-side-effect-rejected.md](gcc-42/gcc-42-contract-side-effect-rejected.md) |
| GCC-43 | An `[[assume]]` whose operand contains a side-effecting contract assertion is rejected instead of declined -- a correct refusal inside a discardable evaluation escapes as a diagnostic | Fixed here | [e929756310e4](https://github.com/notadragon/gnu_gcc/commit/e929756310e479b55a9ab5347cc4bcb1e2c7e18b) | None found (searched 2026-09-16) | [gcc-43-assume-operand-contract-rejected.md](gcc-43/gcc-43-assume-operand-contract-rejected.md) |
| GCC-44 | A violation handler throwing out of a postcondition leaks the returned object | Fixed here | [b259fc5865bd](https://github.com/notadragon/gnu_gcc/commit/b259fc5865bd9898fb9d864b6177fb529fe7a8e7) | [PR127414](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127414) * | [gcc-44-postcondition-throw-leaks-retval.md](gcc-44/gcc-44-postcondition-throw-leaks-retval.md) |
| GCC-45 | A function-contract-specifier binds to the parameter list that precedes it rather than to the declarator, so it is accepted on a typedef, alias, parameter or pointer; silently dropped on a trailing-return type-id; resolved in the return type's parameter scope in the classic spelling; and refused on an array declarator | Fixed here | [bb1e7f9064a1](https://github.com/notadragon/gnu_gcc/commit/bb1e7f9064a185d7fe2fd1a832cc5eb214637ab4), [dc1d155588fa](https://github.com/notadragon/gnu_gcc/commit/dc1d155588fad211f940df916102f076f277abc7) | [PR127572](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127572) * (PR123909 + PR94984 report it for the requires-clause) | [gcc-45-contract-bound-to-wrong-declarator.md](gcc-45/gcc-45-contract-bound-to-wrong-declarator.md) |
| GCC-47 | A requires-clause on a parameter of function type is accepted and silently dropped (not a contracts bug) | Fixed here | [dc1d155588fa](https://github.com/notadragon/gnu_gcc/commit/dc1d155588fad211f940df916102f076f277abc7) | None found (searched 2026-09-16) | [gcc-47-requires-clause-on-parameter.md](gcc-47/gcc-47-requires-clause-on-parameter.md) |
| GCC-49 | A postcondition with a result name on a function whose return type deduction fails aborts the compiler | Fixed here | [d83dc4e8b157](https://github.com/notadragon/gnu_gcc/commit/d83dc4e8b157c7e9ffb09f78a81eb2bd0ab12ff7) | [PR127450](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127450) | [gcc-49-postcondition-undeduced-result-ice.md](gcc-49/gcc-49-postcondition-undeduced-result-ice.md) |
| GCC-50 | `<contracts>` is inert below C++26, so `-fcontracts` there cannot declare a violation handler | Fixed here | [67a777d28425](https://github.com/notadragon/gnu_gcc/commit/67a777d28425ba0d2c92a7aa4be0d9d4ee2e09a8) | None found (searched 2026-09-18) | [gcc-50-contracts-header-inert-below-cxx26.md](gcc-50/gcc-50-contracts-header-inert-below-cxx26.md) |
| GCC-56 | A postcondition that reads its result name is rejected as non-constant during constant evaluation: the result name is never bound, so the static potential-constant check fails before the predicate is evaluated | Fixed here | [b9616010f2c7](https://github.com/notadragon/gnu_gcc/commit/b9616010f2c7bbec799bb12be0973e4e566e260b) | [PR125587](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125587) | [gcc-56-postcondition-result-not-bound-in-constexpr.md](gcc-56/gcc-56-postcondition-result-not-bound-in-constexpr.md) |

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
