# GCC-551: The contract on a block-scope first declaration is dropped (or reported as "added")

**Status:** Fixed here (commit [80065260b084](https://github.com/notadragon/gnu_gcc/commit/80065260b08455b698a22017c35a3d499eae5376)) --
found 2026-09-25 (review round #12 fan-out, g-c-12 and c-b-16)
**Resolved by:** `0200-block-scope-first-decl-contract`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `wrong-code`, `rejects-valid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract block scope`, `declaration adds contracts`,
plus every open bug with `contract` in its summary; nothing about
block-scope declarations.  Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260922` (`f008f03eff8`) show both symptoms; fixed on this branch.
**Workaround:** declare the function (with its contract) at namespace scope
before the block-scope declaration
**Clang:** checks the contract and accepts the repeated form (branch; stock
Clang has no contracts)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] the contract on a block-scope first declaration of a function is dropped, or reported as added by the definition` |

Attachments:

| File | Description |
|---|---|
| [`block-scope-first-decl-contract-dropped.cpp`](block-scope-first-decl-contract-dropped.cpp) | Definition omits the contract: no check |
| [`block-scope-decl-then-def-rejected.cpp`](block-scope-decl-then-def-rejected.cpp) | Definition repeats the contract: rejected |

````
A block-scope function declaration can be the first declaration of the
function, and its function-contract-specifiers then introduce the
function's contract assertions ([dcl.contract.func]).  A later declaration
may omit them or repeat them.  GCC drops them when they are omitted, and
rejects them when they are repeated.

Omitted on the definition:

```
#include <cstdio>

int main ()
{
  void lx (int x) pre (x > 0);
  lx (-1);
  std::puts ("after");
}

void lx (int) {}
```

```
$ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
    block-scope-first-decl-contract-dropped.cpp && ./a.out
after
```

Expected: a contract violation for lx (-1) before "after".  With the
contract moved to a namespace-scope declaration before main (void lx (int
x) pre (x > 0);, and void lx (int x); in the block), the violation is
reported as expected.

Repeated on the definition, which is equally valid:

```
void o (int t)
{
  void loc (int x) pre (x > 0);
  loc (t);
}

void loc (int x) pre (x > 0) {}
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only \
    block-scope-decl-then-def-rejected.cpp
block-scope-decl-then-def-rejected.cpp:13:6: error: declaration adds contracts to 'void loc(int)'
   13 | void loc (int x) pre (x > 0) {}
      |      ^~~~~~~~~~~~~~~~~~~~~~~
block-scope-decl-then-def-rejected.cpp:9:8: note: first declared here
    9 |   void loc (int x) pre (x > 0);
      |        ^~~
```

Expected: accepted.  The note even points at the first declaration, which
has the contract.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

From reading the code, not confirmed in a debugger:
push_local_extern_decl_alias (gcc/cp/name-lookup.cc) creates the
namespace-scope alias of a block-scope extern declaration with copy_decl,
and the block-scope declaration's contract specifiers are not carried to
it.  The later definition merges with the alias, so an omitted contract is
lost and a repeated one is "added" (check_redecl_contract,
gcc/cp/contracts.cc).


VERSIONS -- all on x86_64-linux-gnu

  source              version                       dropped  rejected
  compiler-explorer   16.1.0                        yes      yes
  compiler-explorer   16.2.0                        yes      yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes      yes
````

## Our Fix

`gcc/cp/name-lookup.cc`, `push_local_extern_decl_alias`: a newly created
namespace-scope alias receives the block-scope declaration's contract
specifiers, remapped onto the alias's own parameters with
`copy_and_remap_contracts`.  Regression tests:
`g++.dg/contracts/cpp26/block-scope-first-decl-contract.C` and
`block-scope-decl-then-def-contract.C`.
