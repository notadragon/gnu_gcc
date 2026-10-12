# GCC-553: Templates exported from a module lose their contracts

**Status:** Fixed here (commit [70d06c3ef1af](https://github.com/notadragon/gnu_gcc/commit/70d06c3ef1afc229ab915f50454807e9ee0d9b2d))
**Resolved by:** `0230-module-template-contracts`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract module template` and `contracts modules`,
plus every open bug with `contract` in its summary; only PR124264, a MinGW
linkage problem, involves both.  Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) behave identically; fixed on this branch.
**Workaround:** none for templates instantiated in the importer; an
explicit instantiation in the module interface unit keeps the check for that
specialization
**Clang:** checks all three with a one-step module build (branch; stock
Clang has no contracts).  Its two-step build (`--precompile`, then `-c
m.pcm`) asserts in `ContractOptions::initConfig` ("initialized without a
DiagnosticsEngine") -- a separate Clang issue, possibly the same as CLANG-520
in that fork (the libclang/ASTUnit path to that assertion)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts][modules] contracts of exported templates are lost in an importer that instantiates them` |

Attachments:

| File | Description |
|---|---|
| [`gcc-553-files/m.cpp`](gcc-553-files/m.cpp) | Module interface |
| [`gcc-553-files/main.cpp`](gcc-553-files/main.cpp) | Importer |

````
A function template, or a member function of a class template, exported
from a module keeps its precondition in the module's own translation unit
but loses it in an importer that instantiates it.  A non-template function
is fine.

```
// m.cpp
export module m;
export template <class T> T g1 (T x) pre (x > 0) { return x; }
export template <class T> struct TS { T k (T x) pre (x > 0) { return x; } };
export int nn (int x) pre (x > 0) { return x; }
```

```
// main.cpp
import m;
#include <cstdio>
int main ()
{
  std::puts ("g1"); g1 (-1);
  std::puts ("TS"); TS<int>{}.k (-1);
  std::puts ("nn"); nn (-1);
  std::puts ("end");
}
```

```
$ g++ -std=c++26 -fmodules -fcontracts \
    -fcontract-evaluation-semantic=observe -c m.cpp
$ g++ -std=c++26 -fmodules -fcontracts \
    -fcontract-evaluation-semantic=observe main.cpp m.o
$ ./a.out
g1
TS
nn
contract violation in function int nn(int) at m.cpp:9: x > 0
[assertion_kind: pre, semantic: observe, mode: predicate_false, terminating: no]
end
```

Expected: a violation for g1 (-1) and TS<int>::k (-1) as well.  An explicit
instantiation in the interface unit keeps the check for that
specialization.


DISCOVERY

Found while reviewing an implementation of C++26 contracts for how
contracts cross module boundaries.


ANALYSIS

From reading the code, not confirmed in a debugger: trees_out::fn_parms
(gcc/cp/module.cc) walks get_fn_contract_specifiers only when
!streaming_p () (for the dependency graph) and never writes them, and
trees_in::fn_parms_init reads only the pre/post function references.  An
importer instantiating the template therefore has no contracts to
substitute.  Streaming the contract specifiers with the declaration and
calling set_fn_contract_specifiers on read would fix it.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       template contracts lost
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Our Fix

`gcc/cp/module.cc`: `trees_out::fn_parms` writes the contract specifiers with
the parameters, and `trees_in::fn_parms_init` sets them on the declaration it
reads (a declaration that merges with an existing one is discarded, and the
existing one keeps its own).  Regression test:
`g++.dg/modules/contract-template-export_a.C` / `_b.C`.

## Notes (ours, not for the report)

No `verify-cases.txt` row: the reproducer needs two translation units and a
link, and `verify.sh` measures one file per row.  Clang mirror:
`clang/test/Contracts/module-template-contract-export.cpp`.
