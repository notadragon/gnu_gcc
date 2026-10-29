# GCC-596: a lambda in one member's precondition and a bad result name in a member template crash after the error

**Kind:** defect (ice-on-invalid-code)
**Status:** Open (upstream); fixed here
**Resolved by:** not yet identified -- this branch reports the error and
carries on; which commit made the difference has not been bisected
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-invalid-code`
**Upstream Link:** None found (searched 2026-09-30: Bugzilla REST quicksearch
on `contracts lambda member template`, `contracts lambda precondition ICE`,
`does not return a value to test`)
**Affects:** measured 2026-09-30 -- stock trunk `17.0.0 20260922`
(checking enabled) prints the error, then "confused by earlier errors,
bailing out".  This branch prints the error only.
**Found:** review round #12 (c-b-17), as the standard's [expr.prim.id.unqual]
example with `g` instantiated; reduced 2026-09-30.

## Bug Report

Not filed yet: a checking-build backtrace would make it a better report.

| File | Description |
|---|---|
| [`member-lambda-pre-template-post-ice.cpp`](member-lambda-pre-template-post-ice.cpp) | Four lines |

```
struct Y {
  void f () pre ([] { return true; } ());
  template <int N> void g () post (r : r);
};
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only t.cpp
t.cpp:3:30: error: function does not return a value to test [-Wtemplate-body]
t.cpp:4: confused by earlier errors, bailing out
```

The error is correct.  Each half alone is fine: without the lambda
(`pre (true)`), or with `g` not a template, or at namespace scope, there is
no crash; the order of the two members does not matter.  Class scope is what
defers both members' contracts to the end of the class.
