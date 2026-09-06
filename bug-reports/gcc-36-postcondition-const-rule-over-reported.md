# GCC-36: the postcondition const-parameter rule is reported twice

**Status:** Open on stock upstream
**Component:** c++ / contracts
**Upstream Link:** None found (searched 2026-09-06)
**Reproduces on stock:** GCC **16.2.0** and **trunk** (Compiler Explorer build
`3653b8dd68b0`, 17.0.0 20260905). GCC 15.3.0 predates contracts and rejects the
syntax outright, so 16 is the first affected release.
**Found:** 2026-09-06, while fixing GCC-35.

## Symptom

The [dcl.contract.func] rule -- "if the predicate of a postcondition assertion
of a function *f* odr-uses a non-reference parameter of *f*, that parameter ...
shall have const type" -- is diagnosed twice for one mistake. Two shapes, and
they are not the same bug wearing two hats.

### A. Function template -- two *different* diagnostics

[`gcc-36a-postcondition-const-template.C`](gcc-36a-postcondition-const-template.C)

```c++
template <class T> T f (T v) post (r: v == r) { return v; }
int use () { return f (1); }
```

```
:1:27: error: value parameter 'v' used in a postcondition must be const
:1:39: error: a value parameter used in a postcondition must be const
```

Two mechanisms both implement the rule, and an instantiation is a
redeclaration of its pattern, so both run:

* `check_postcondition_parm_in_redecl` (`cp/contracts.cc`) carries the
  "odr-used in a postcondition" property from one declaration to the next and
  checks constness on the way;
* `check_postcondition_odr_use_r` (`cp/contracts.cc`) walks the substituted
  predicate and checks each parameter it finds odr-used.

`check_postcondition_redecl_parm_types`, a third entry point, already carries
an explicit "do not say it twice" guard, so not duplicating is the established
intent -- this pair simply misses each other.

### B. Non-template -- the *same* diagnostic, twice

[`gcc-36b-postcondition-const-nontemplate.C`](gcc-36b-postcondition-const-nontemplate.C)

```c++
int f (int v) post (r: v == r) { return v; }
```

```
:1:24: error: a value parameter used in a postcondition must be const
:1:12: note: parameter declared here
:1:24: error: a value parameter used in a postcondition must be const
:1:12: note: parameter declared here
```

Byte-identical, twice. This one is **stock only**.

## What reproduces where

| shape | stock 16.2.0 / trunk | contracts-p3850 |
|---|---|---|
| A, function template | 2 different errors | **same 2 errors** |
| B, non-template | 2 identical errors | **1 error** |

Our branch reports B once, and at the contract (column 15) rather than at the
odr-use (column 24), having diverged in `check_postcondition_odr_use_r` -- it
honours a contract location when one is available. So B is upstream's alone.
It is recorded here anyway: the entry criterion for this directory is
reproduction on stock, whether or not the branch still has it.

A is still present on the branch and therefore also has a row in
[`../open-issues/README.md`](../open-issues/README.md).

## A fix that does NOT work

For shape A the obvious guard -- in `check_postcondition_odr_use_r`, skip the
error when `parm_used_in_post_p (t)` was already set on entry, reasoning that
only the carry-over could have set it and the carry-over diagnoses as it goes
-- **is wrong**, and cost a full build-and-test cycle to find out. It silenced
65 tests' worth of real diagnostics.

The two checks do not test the same condition. The carry-over's guard is

```c
!dependent_type_p (TREE_TYPE (t2)) && !CP_TYPE_CONST_P (TREE_TYPE (t2))
  && !TREE_READONLY (t2)
```

while the walk's is only `!dependent_type_p && !CP_TYPE_CONST_P`. Contract
constification sets `TREE_READONLY`, so for a constified parameter the
carry-over **marks without diagnosing** and the walk is the only thing that
reports. "Marked" therefore does not imply "reported".

A real fix needs a separate "already diagnosed" bit on the `PARM_DECL`
(`DECL_LANG_FLAG_4` is taken by `parm_used_in_post`, so this means finding a
free one), or making the two checks agree on one condition with a single owner.

## Bugzilla

Searched 2026-09-06 by summary substring `postcondition`: 18 PRs, none about a
duplicated diagnostic. The nearest neighbours are all different symptoms in the
same code: **PR124395** (ICE in `check_postconditions_in_redecl` instantiating
a function template), **PR127196** (redeclaration with a type-dependent
parameter in a postcondition accepted) and **PR126897** (parameters in the left
operand of a comma). Quicksearch on the diagnostic text returns nothing.

## Impact

Diagnostic quality only. Both messages are true and the program is correctly
rejected either way.
