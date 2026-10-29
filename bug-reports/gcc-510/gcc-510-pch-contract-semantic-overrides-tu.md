# GCC-510: A precompiled header's contract evaluation semantic silently overrides the TU's

**Status:** Fixed here (commit [83c13da929a0](https://github.com/notadragon/gnu_gcc/commit/83c13da929a08467390e8171ae516e9b34b81df8))
**Resolved by:** `0210-pch-contract-semantic`
**Component:** pch (c++)
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract pch`, `contract precompiled` and
`contract-evaluation-semantic pch`, plus every open bug with `contract` in
its summary; nothing about PCH.  Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`); fixed on this branch.  The PCH's
semantic wins, even with `-Winvalid-pch`, at run time and in constant
evaluation.  Clang uses the TU's flag (found in review round #12 fan-out,
c-c-17).

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `pch` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a PCH built with a different -fcontract-evaluation-semantic= is used silently, and its semantic wins` |

Attachments:

| File | Description |
|---|---|
| [`h.h`](h.h) | Header with an inline function carrying a precondition |
| [`m.cpp`](m.cpp) | TU that violates the precondition |

````
A header precompiled with -fcontract-evaluation-semantic=ignore is used by
a translation unit compiled with -fcontract-evaluation-semantic=enforce, and
the header's semantic wins.

```
// h.h
inline int f (int x) pre (x > 0) { return x; }
```

```
// m.cpp
#include <cstdio>
int main () { f (-1); std::puts ("returned"); }
```

```
$ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore \
    -x c++-header h.h -o h.h.gch
$ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce \
    -Winvalid-pch -include h.h m.cpp && ./a.out
returned
```

Without h.h.gch the same second command terminates at the violated
precondition:

```
contract violation in function int f(int) at ./h.h:1: x > 0
[assertion_kind: pre, semantic: enforce, mode: predicate_false, terminating: yes]
terminate called without an active exception
```

-H shows the PCH is used ("! ./h.h.gch") and -Winvalid-pch says nothing.

The same happens in constant evaluation.  With

  // h.h
  constexpr int f (int x) pre (x > 0) { return x; }
  // m.cpp
  constexpr int v = f (-1);

the TU compiled with enforce against the ignore PCH is accepted, while
without the PCH it is rejected with "contract predicate is false in constant
expression".

Expected: either the PCH is rejected as invalid when the contract
evaluation semantic differs, or the semantic is taken from the TU.


DISCOVERY

Found while reviewing an implementation of C++26 contracts for options that
a precompiled header can carry.


ANALYSIS

From reading the code, not confirmed in a debugger: the evaluation semantic
is resolved while the header is parsed and saved in the contract trees, and
the PCH validity check (c_common_valid_pch) does not cover
-fcontract-evaluation-semantic=.  Rejecting a PCH built with different
contract options is the simplest fix; re-resolving the semantic at use is
the other.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       PCH semantic wins
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Our Fix

`gcc/c-family/c-pch.cc`: `-fcontract-evaluation-semantic=` joins
`-fexceptions` in `pch_matching`, so a PCH built with another semantic is
not used and the header is read as source.  Regression test:
`g++.dg/pch/contract-semantic-mismatch.C` (+ `.Hs`).

## Notes (ours, not for the report)

On the branch the PCH option check still misses
`-fcontract-configuration-file=` / `-fcontract-group-evaluation-semantic=`
(GCC-614 in open-issues/).

No `verify-cases.txt` row: the case needs two invocations (precompile the
header, then compile the TU), and verify.sh measures one file per row.
