# GCC-27: A friend declaration's contract is never checked against an earlier declaration

**Status:** Open -- **deliberately**, on this branch; see below
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `accepts-invalid`, `diagnostic`
**Upstream Link:** [PR127291](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127291)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: nothing matched.)
**Affects:** measured 2026-09-09 -- accepted silently on 16.1.0, 16.2.0 and
both trunk builds.

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
in this directory, which carries both cases. Two controls were also
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

**None, by decision.** Both routes above are invasive for a degenerate
construct, so the limitation is retained rather than overlooked;
investigated 2026-08-07.

It is pinned in the testsuite by
`gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C`,
whose `xfail` accounts for both of the contracts suite's expected failures
(one per `-std` variant). A fix -- ours or upstream's -- turns that into an
XPASS, which is the signal to delete this entry and the `xfail` together.
That test only covers Case 1 (two friend declarations); it predates the
discovery that Case 2 (an ordinary declaration redeclared by a friend)
reaches the identical defect, and has not been extended to pin it.

Because it is unfixed here as well as upstream, it also has a row in
[`../open-issues/README.md`](../../open-issues/README.md), which links back to
this writeup rather than repeating it.

## Notes

Clang has the same defect, tracked as CLANG-12 in the companion Clang fork,
[notadragon/llvm-project](https://github.com/notadragon/llvm-project).
That one is
branch-only and unfilable -- contracts are not upstream in Clang -- so this
report stands alone, but the two should be fixed with an eye on each other.

Originally recorded as branch-only in `open-issues/` and moved here
2026-09-05 on measuring that it reproduces on stock. The entry criterion is
reproduction on stock, and it had never been tested.
