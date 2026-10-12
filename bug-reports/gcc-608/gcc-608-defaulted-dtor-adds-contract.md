# GCC-608: a defaulting redeclaration that adds a contract crashes after its error

**Kind:** defect (ice-on-invalid-code)
**Status:** Open (upstream); fixed here
**Resolved by:** not yet identified -- this branch reports the error and
carries on; which commit made the difference has not been bisected
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-invalid-code`
**Upstream Link:** None found for this shape (searched 2026-09-30); likely a
sibling of [PR125403](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125403)
(ICE in maybe_apply_function_contracts, postcondition on an in-class
defaulted constructor) and related to
[PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486)
**Affects:** measured 2026-09-30 -- stock trunk `17.0.0 20260922`
(checking enabled) prints the error, then "confused by earlier errors,
bailing out".  This branch prints the error only.

## Bug Report

Not filed: a checking-build backtrace is needed first, to tell whether this
is PR125403.

| File | Description |
|---|---|
| [`defaulted-dtor-adds-contract.cpp`](defaulted-dtor-adds-contract.cpp) | An out-of-line `= default` redeclaration of a destructor that adds a precondition |

```
struct S { ~S (); };
S::~S () pre (true) = default;
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only t.cpp
t.cpp:2:1: error: declaration adds contracts to 'S::~S()'
t.cpp:2: confused by earlier errors, bailing out
```

The error is correct; "confused by earlier errors" is how GCC reports an
internal error that happens after an error has been issued.
