# GCC-27: A friend declaration's contract is never checked against an earlier declaration

**Status:** Open upstream, both cases. **Half fixed on this branch** as of
2026-09-20: case 1 is diagnosed here, case 2 is not -- see "Our Fix".
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `accepts-invalid`, `diagnostic`
**Upstream Link:** [PR127291](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127291)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: nothing matched.)
**Affects:** measured 2026-09-09 -- accepted silently on 16.1.0, 16.2.0 and
both trunk builds. Re-measured 2026-09-20 against stock trunk 20260919:
**both cases still accepted silently there**, so nothing here is fixed
upstream.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] friend declarations do not validate matching contract assertions` |

Attachments:

| File | Description |
|---|---|
| [`deferred-friend-contract-mismatch.C`](deferred-friend-contract-mismatch.C) | Two independent cases -- two friend declarations, and an ordinary declaration redeclared by a friend -- each accepted with no diagnostic |

````
A friend declaration's contract is never validated against an earlier
declaration of the same function -- although a friend declaration that is
the first declaration will introduce the contract assertions that later
(non-friend) declarations will be compared against.  Two shapes reach
this defect, and neither is diagnosed:

```
// Case 1: two friend declarations of the same function.
struct C {
    friend int f(int x) pre(x > 0);
    friend int f(int x) pre(x < 0);   // accepted; should be a mismatch
};

int f(int x) { return x; }

// Case 2: an ordinary declaration, then a friend redeclaration with a
// different contract -- the friend need not come first, nor does the
// earlier declaration need to be a friend itself.
int h(int x) pre(x > 0);

struct D {
    friend int h(int x) pre(x < 0);   // accepted; should be a mismatch
};

int h(int x) { return x; }
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -c deferred-friend-contract-mismatch.C
$ echo $?
0
```

No diagnostic at all, for either case.  Only the first contract seen for
each function takes effect -- `pre(x > 0)` for both `f` and `h` here.

Non-friend declarations are checked against the first declaration
consistently.

DISCOVERY

Found while implementing redeclaration matching for contracts, checking the
matcher against each path by which one function can be declared twice.  The
friend-inside-a-class path was the one that had no coverage. The ordinary-
declaration-then-friend shape (Case 2 above) was found afterward, while
confirming exactly which redeclaration orderings the defect covers.

ANALYSIS

A friend declaration's contract is still DEFERRED_PARSE at the point
duplicate_decls merges it with the earlier declaration -- a contract on a
member or friend declaration is late-parsed once the class is complete --
and check_redecl_contract skips matching when either side is still
deferred.

Deferring the comparison instead is not the small change it looks like.
duplicate_decls discards whichever declaration is being merged away and
calls remove_decl_with_fn_contracts_specifiers on it, dropping its deferred
contract entirely, before end-of-class late-parsing happens. This is
independent of what the earlier declaration was: an ordinary declaration
discards the friend's contract the same way a first friend declaration
does. By the time the tokens could be parsed there is nothing left to
compare them against -- unless the friend was first, in which case its
own contract has already been late-parsed at end-of-class before anything
later could discard it.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepted silently
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260909, 919c0d16c91  yes
  local build -g      17.0.0 20260909, 7dab38c9d71  yes

```
$ ./gcc-16.2.0/bin/g++ -v
Using built-in specs.
COLLECT_GCC=./gcc-16.2.0/bin/g++
COLLECT_LTO_WRAPPER=.../gcc-16.2.0/bin/../libexec/gcc/x86_64-linux-gnu/16.2.0/lto-wrapper
Target: x86_64-linux-gnu
Configured with: ../gcc-16.2.0/configure --prefix=/opt/compiler-explorer/gcc-build/staging --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --disable-bootstrap --enable-multiarch --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --enable-clocale=gnu --enable-languages=c,c++,fortran,ada,objc,obj-c++,go,d,m2,rust,cobol,algol68 --enable-ld=yes --enable-gold=yes --enable-libstdcxx-time=yes --enable-linker-build-id --enable-lto --enable-plugins --enable-threads=posix --with-pkgversion=Compiler-Explorer-Build-gcc--binutils-2.44
Thread model: posix
Supported LTO compression algorithms: zlib
gcc version 16.2.0 (Compiler-Explorer-Build-gcc--binutils-2.44)
```
````

## Reproducer

See [`deferred-friend-contract-mismatch.C`](deferred-friend-contract-mismatch.C)
in this directory, which carries both cases, selected by `-DCASE=1` and
`-DCASE=2`. They are measured as two rows (`GCC-27a`, `GCC-27b`) because this
branch now diagnoses case 1 and not case 2: measured as one file the branch
column reads `error` off case 1 alone and says nothing about the half that is
still broken. Two controls were also
verified but are not reproduced here: two plain, non-friend declarations
of one function (caught normally), and a friend declared first, later
redeclared by an ordinary declaration (also caught) -- together showing
that only a friend declaration which is *not* a function's first
declaration reaches the defect.

## Root cause

A friend declaration's contract is `DEFERRED_PARSE` at the point
`duplicate_decls` merges it with the earlier declaration, and
`check_redecl_contract` skips matching when either side is still deferred.

Deferring the comparison instead is not the small change it looks like.
`duplicate_decls` discards whichever declaration is being merged away and
calls `remove_decl_with_fn_contracts_specifiers` on it -- dropping its
`contract_decl_map` entry -- **before** end-of-class late parsing runs. This
does not depend on the earlier declaration being a friend: an ordinary
declaration discards a later friend's contract the same way a first friend
declaration would. The discarded contract's tokens are gone by the time
anything could compare them -- unless the friend was first, in which case
its contract has already been late-parsed at end-of-class before anything
later could discard it. A fix has to either

* compare the raw deferred token streams at the skip point, which is a
  textual match that diverges from the semantic matcher (it would report
  `pre ((x > 0))` against `pre (x > 0)`), or
* keep the discarded declaration's tokens and late-parse them in its own
  parameter scope, since the two declarations may name their parameters
  differently and the surviving declaration's scope cannot be reused.

## Our Fix

**Case 1 only.** The declarator-position work made every function contract
deferred, which forced the deferred-match queue to exist: `check_redecl_contract`
records the two contract vectors together with the redeclaration's parameter
*names*, and `flush_deferred_contract_matches` parses the reclaimed side
against the surviving declaration's parameters, bound under the names the
redeclaration itself wrote so a renamed parameter resolves positionally. Two
friend declarations of one function are merged by `duplicate_decls`, so they
reach that queue and are now diagnosed. It is pinned by
`gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C`,
which is a positive test -- it was the `xfail` that used to account for two of
the suite's expected failures, and the count fell from 9 to 7 when it flipped.

**Case 2 is still open here**, and was not addressed by that work. The
separate `check_redecl_contract` call in `do_friend` covers only the
*qualified* friend path, where `check_classfn` resolves the redeclaration and
`duplicate_decls` never runs; its own comment says so. A friend that
redeclares an ordinary **namespace-scope** function takes neither route -- not
the member path, and not the queue -- so nothing compares its contract.
Measured 2026-09-20: accepted with no diagnostic on this branch.

Because case 2 is unfixed here as well as upstream, it keeps a row in
[`../open-issues/README.md`](../../open-issues/README.md), narrowed to that
case, which links back to this writeup rather than repeating it. Case 1 has no
`open-issues` row any more -- it is not broken here -- but stays in this
directory, because `bug-reports/` tracks what reproduces on **stock** and a row
survives until *upstream* fixes it. Both halves are still open under PR127291.

## Notes

Clang has the same defect, tracked as CLANG-12 in the companion Clang fork,
[notadragon/llvm-project](https://github.com/notadragon/llvm-project).
That one is
branch-only and unfilable -- contracts are not upstream in Clang -- so this
report stands alone, but the two should be fixed with an eye on each other.

Originally recorded as branch-only in `open-issues/` and moved here
2026-09-05 on measuring that it reproduces on stock. The entry criterion is
reproduction on stock, and it had never been tested.
