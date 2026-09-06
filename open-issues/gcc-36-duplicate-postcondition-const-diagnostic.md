# GCC-36: [dcl.contract.func]/7 is reported twice for a function template

**Status:** Open (defect -- diagnostic quality)
**Kind:** defect
**Component:** c++ / contracts
**Upstream Link:** UNKNOWN -- plain `-fcontracts` reaches this, so it may well
reproduce on stock trunk; not measured yet.
**Found:** 2026-09-06, while fixing GCC-35.

## Symptom

One mistake, two errors:

```c++
template <class T> T f (T v) post (r: v == r) { return v; }
int use () { return f (1); }
```

```
:1:27: error: value parameter 'v' used in a postcondition must be const
:1:41: error: a value parameter used in a postcondition must be const
```

Reproducer:
[`gcc-36-duplicate-postcondition-const-diagnostic.C`](gcc-36-duplicate-postcondition-const-diagnostic.C).
A non-template function reports once; only templates double up.

## Why

Two mechanisms implement the rule and both fire for a template:

* `check_postcondition_parm_in_redecl` (`contracts.cc`, error at ~1915) carries
  the "odr-used in a postcondition" property from one declaration to the next
  and checks constness as it goes. An instantiation *is* a redeclaration of its
  pattern, so this runs for every instantiation.
* `check_postcondition_odr_use_r` (`contracts.cc`, error at ~1756) walks the
  substituted predicate and checks each parameter it finds odr-used.

Neither is wrong; they simply overlap. `check_postcondition_redecl_parm_types`
(a third entry point) already carries a "do not say it twice" guard, so
avoiding duplication is the established intent here -- this pair just misses
each other.

## Pre-existing, and why it surfaced now

Not introduced by GCC-35. Measured on a pre-GCC-35 compiler: the reproducer
above already produced both errors. What GCC-35 changed is *where* it is
visible -- a declared-only template's contracts are now substituted at its
odr-use, so the walk now runs for those too, and
`deducing-this-postcondition-param.C` started reporting the rule twice on a
declared-only member template. That test now carries both expectations with a
comment pointing here; when this is fixed, one of them goes away.

## A fix that does NOT work

The obvious one -- in `check_postcondition_odr_use_r`, skip the error when
`parm_used_in_post_p (t)` was already set on entry, on the theory that only the
carry-over could have set it and the carry-off diagnoses as it goes -- **is
wrong, and costs a full build and test cycle to discover.** It suppressed 65
tests' worth of diagnostics.

The premise fails because the two checks do not test the same condition. The
carry-over's guard is

```c
!dependent_type_p (TREE_TYPE (t2)) && !CP_TYPE_CONST_P (TREE_TYPE (t2))
  && !TREE_READONLY (t2)
```

while the walk's is only `!dependent_type_p && !CP_TYPE_CONST_P`. Contract
constification sets `TREE_READONLY` on the parameter, so for a constified
parameter the carry-over **marks without diagnosing** and the walk is the only
thing that reports. "Marked" therefore does not imply "reported", and a guard
built on that premise silences the sole diagnostic for the commonest shape.

A real fix needs to distinguish the two -- either a separate "already
diagnosed" bit on the `PARM_DECL` (`DECL_LANG_FLAG_4` is taken by
`parm_used_in_post`, so this means finding a free one), or making the two
checks agree on a single condition and giving exactly one of them ownership.

## Impact

Diagnostic quality only; both messages are true and the program is correctly
rejected. Worth fixing because the duplication is confusing and because GCC's
own code already tries to avoid it elsewhere.
