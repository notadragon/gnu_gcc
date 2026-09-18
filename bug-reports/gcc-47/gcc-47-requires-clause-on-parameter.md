# GCC-47: A requires-clause on a parameter of function type is accepted and silently dropped

**Status:** Fixed here
**Resolved by:** `0160-requires-clause-on-parameter`
**Component:** c++ / concepts
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found for this position (searched 2026-09-16:
`requires-clause`, `requires-clause parameter`, `"requires clause" parameter`,
`requires declarator`, `requires-clause accepted`, `requires-clause ignored`,
`requires-clause function-typed`, `constraint function pointer parameter`,
`keywords:accepts-invalid requires`, all `component:c++`).

**But there are two open siblings, and they should be read with this one** --
the family is "where may a requires-clause appear on a declarator", and GCC
is wrong at three different points in it:

* [PR123909](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123909)
  (UNCONFIRMED, 2026-01-31, keywords `accepts-invalid` `rejects-valid`) --
  `void (*g () requires true) ();` is accepted and
  `void (*f ()) () requires true;` is rejected, both backwards.  The
  submitter's diagnosis is the same as this one's: GCC treats the clause as
  part of the parameters-and-qualifiers inside the declarator rather than as
  following the complete declarator.
* [PR94984](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94984) (NEW,
  2020-05-07) -- `char (*f (int i))[N] requires (...)` rejected, a
  rejects-valid in the same area.

**This is a different position from either, and the fix does not touch
them.** Measured 2026-09-16 against the fixed branch build: both of
PR123909's cases behave exactly as they do on stock trunk -- its case 1 still
rejected, its case 2 still accepted.  So the parameter hole is not a
duplicate and is not incidentally closed, but whoever files this should say
so, and may reasonably prefer to add it as a comment on PR123909 rather than
open a fourth report on one rule.
**Affects:** measured 2026-09-16 -- accepted on `13.4.0`, `14.4.0`, `15.3.0`,
`16.1.0`, `16.2.0` and trunk `17.0.0 20260914 (experimental)`
(`b76fde4b175`). Long-standing rather than a regression; it appears to date
from the introduction of the checks for the sibling placements.

**Not a contracts bug.** It is recorded here because it was found alongside
one: the same ordering in `grokdeclarator` lets a C++26
function-contract-specifier through on the identical declarator (GCC-45 and
GCC-46).

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++20][concepts] a requires-clause on a function-typed parameter is accepted and silently ignored` |

Attachments:

| File | Description |
|---|---|
| [`requires-clause-on-parameter.cpp`](requires-clause-on-parameter.cpp) | Five shapes, all accepted with no diagnostic |

````
A requires-clause written on a parameter whose declared type is a function
type is accepted and then silently discarded.

Such a parameter is adjusted to a pointer to function ([dcl.fct]/5), so it
declares no function and there is nothing for the clause to constrain.  The
program should be rejected, and every other misplaced requires-clause is.

```
template <class T> concept C = true;

void f (int bar () requires true);

void g (int bar () requires true) { (void) bar; }

void h (int bar (int, int) requires true);

template <class T>
void t (T bar () requires C<T>);

struct S {
  void m (int bar () requires true);
};
```

```
$ g++ -std=c++20 -fsyntax-only requires-clause-on-parameter.cpp; echo $?
0
```

For contrast, the sibling placements ARE diagnosed, which is what makes this
one look like an oversight rather than a decision:

```
typedef int Tdef (int) requires true;    // error: requires-clause on typedef
using Alias = int (int) requires true;   // error: requires-clause on type-id
int (*ptr) (int) requires true;          // error: requires-clause on declaration
                                         //        of non-function type
int (*ret (int)) (int) requires true;    // error: requires-clause on return type
```


ANALYSIS

The check that should catch this exists and is simply ordered before the
adjustment that would make it fire.

grokdeclarator (gcc/cp/decl.cc) accumulates the clause from the declarator
walk into its local `reqs`, and diagnoses a misplaced one at four points: the
typedef return, the type-id return, the `case cdk_function:` arm (the
"on return type" case), and

  if (!FUNC_OR_METHOD_TYPE_P (type))
    {
      ...
      if (reqs)
        error_at (location_of (reqs),
                  "requires-clause on declaration of non-function type %qT",
                  type);
    }

That last one is exactly the right question for a parameter, and it does not
fire.  At that point `type` is still the bare FUNCTION_TYPE: the
function-to-pointer adjustment for parameters happens further down, in the
`decl_context == PARM` handling --

  else if (TREE_CODE (type) == FUNCTION_TYPE)
    type = build_pointer_type (type);

-- after the guard that depends on it.  grokdeclarator then builds a
PARM_DECL and `reqs` is never read again.

So the fix is not to add a fifth check but to ask about the context rather
than the type at the fourth: `decl_context == PARM` (and `CATCHPARM`) is true
regardless of where the decay has got to.

The same ordering is why a C++26 function-contract-specifier on the same
declarator is also accepted and dropped, which is how this was noticed.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepted
  compiler-explorer   13.4.0                        yes
  compiler-explorer   14.4.0                        yes
  compiler-explorer   15.3.0                        yes
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260914, b76fde4b175  yes


DISCOVERY

Found while fixing the C++26 contracts equivalent, where a contract specifier
on the same declarators is accepted and dropped the same way.  The
requires-clause and the contract travel in the same declarator slot and are
diagnosed at the same four points, so auditing one audited the other.
````

## Reproducer

See [`requires-clause-on-parameter.cpp`](requires-clause-on-parameter.cpp) in
this directory.  No headers, so no `.ii` is needed.

## Our Fix

`gcc/cp/decl.cc`: one check after the declarator walk completes, testing
`decl_context` rather than the type.

It shares its classification with the contracts fix -- a
`classify_non_function_declarator` helper that answers "what does this
declarator declare, if not a function?" once, for both specifiers, so their
answers cannot drift apart.  Only the PARAMETER position is acted on here:
the typedef, type-id and non-function-type positions already have
requires-clause diagnostics further down, and this change deliberately leaves
their wording alone rather than routing them through the new path.  That
keeps the diff to the one position that is actually broken.

Test: `gcc/testsuite/g++.dg/concepts/requires-clause-on-parameter.C`, which
also pins the four sibling placements so that a change to one message does
not quietly diverge from the others.

## Notes

The row stays here until upstream fixes it, per this directory's entry
criterion; `Status: Fixed here` says only that this branch no longer
reproduces it.
