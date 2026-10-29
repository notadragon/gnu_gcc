# GCC-517: A postcondition result name that conflicts with a parameter is diagnosed only in a checked definition

**Status:** Fixed here (commit [b6eece227f40](https://github.com/notadragon/gnu_gcc/commit/b6eece227f40af201fe7903ceb32a6fc8cd46985))
**Resolved by:** `1000-p2900-fixes`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `postcondition result name shadows`, `result name
shadows`, plus every open bug with `contract` in its summary; PR127284 and
PR127287 mention result names but are not this.  Earlier search 2026-09-25,
same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch behave identically: case
1 is diagnosed under the default semantic but not under
`-fcontract-evaluation-semantic=ignore`, and cases 2 and 3 are never
diagnosed
**Clang:** diagnoses cases 1 and 2 under every semantic ("declaration of
result name 'r' shadows parameter"); misses case 3 (CLANG-541 in that fork)
**Found:** 2026-09-25, review round #12 fan-out (g-a-2)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a postcondition result name that conflicts with a parameter is only diagnosed in a checked function definition` |

Attachments:

| File | Description |
|---|---|
| [`result-name-shadows-parameter.cpp`](result-name-shadows-parameter.cpp) | Three conflicting result names, selected with `-DCASE=1..3` |

````
[basic.scope.contract]/2: if a result-name-introducer that is not
name-independent potentially conflicts with a declaration whose target
scope is the function parameter scope of F or, for a lambda, the nearest
enclosing lambda scope, the program is ill-formed.  The rule does not
depend on the evaluation semantic or on whether the declaration is a
definition, but GCC only checks it when it starts a definition whose
contracts are checked.

```
#if CASE == 1
int f (int r) post (r: r > 0) { return r; }             // definition, ignore
#elif CASE == 2
int g (int r) post (r: r > 0);                            // declaration only
#elif CASE == 3
auto l = [x = 1] (int y) post (x: x > 0) { return y; };   // init-capture
#endif
```

Case 1 is diagnosed under the default semantic:

```
$ g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=1 \
    result-name-shadows-parameter.cpp
result-name-shadows-parameter.cpp: In function 'int f(int)':
result-name-shadows-parameter.cpp:12:21: error: contract postcondition result name shadows a function parameter
   12 | int f (int r) post (r: r > 0) { return r; }             // definition, ignore
      |               ~~~~~~^~~~~~~~~
result-name-shadows-parameter.cpp:12:12: note: parameter declared here
   12 | int f (int r) post (r: r > 0) { return r; }             // definition, ignore
      |        ~~~~^
```

but not otherwise; each of these compiles without a diagnostic:

```
$ g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=1 \
    -fcontract-evaluation-semantic=ignore result-name-shadows-parameter.cpp
$ g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=2 \
    result-name-shadows-parameter.cpp
$ g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=3 \
    result-name-shadows-parameter.cpp
```

Expected: an error for each of the three cases under every evaluation
semantic.  In case 3 the conflict is with the lambda's init-capture, whose
target scope is the lambda scope.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

From reading the code: start_function_contracts (gcc/cp/contracts.cc) does
the check, but only after handle_contracts_p, which is false under ignore
and while processing a template, and it runs only when a definition starts.
It also looks only for a PARM_DECL of the function, never at a lambda's
captures.  The check belongs where the postcondition is built, independent
of the semantic, and needs to include the lambda scope.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       1     1 ignore  2     3
  compiler-explorer   16.1.0                        error clean     clean clean
  compiler-explorer   16.2.0                        error clean     clean clean
  compiler-explorer   17.0.0 20260922, f008f03eff8  error clean     clean clean
````

## Our Fix

`gcc/cp/contracts.cc`: `make_postcondition_variable` calls the new
`check_result_name_conflict` before it pushes the result name, so the rule is
checked wherever a postcondition is parsed -- under every semantic, on a
declaration that is not a definition, and in a template.  A conflict is a
parameter bound in the nearest function parameter scope outside the contract
scope (for a lambda, whose contracts are parsed inside its body, a parameter
of the lambda's call operator) or an init-capture of the lambda; a simple
capture declares nothing and does not count, and a result name `_` is
name-independent.  The check that `start_function_contracts` made only for a
checked definition is removed.  Regression test:
`g++.dg/contracts/cpp26/result-name-shadows-parameter.C`.

## Notes (ours, not for the report)

Clang mirrors: `clang/test/Contracts/result-name-shadows-parameter.cpp`
(cases 1 and 2) and `OpenBugs/result-name-shadows-lambda-capture.cpp` (XFAIL,
case 3, CLANG-541).
