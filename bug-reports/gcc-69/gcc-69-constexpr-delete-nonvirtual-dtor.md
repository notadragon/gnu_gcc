# GCC-69: Constant evaluation accepts `delete` through a base pointer whose destructor is not virtual

**Kind:** defect (accepts-invalid)
**Status:** Open (upstream); fixed here 2026-09-30 -- found 2026-09-25
(review round #12, B-misc#13, from the celink out-of-scope drift check)
**Resolved by:** [a8b2518a23de](https://github.com/notadragon/gnu_gcc/commit/a8b2518a23de0f9dcdc9d3529b4b0be172e508aa)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** [PR97647](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97647)
(NEW, confirmed 2021; PR122336 was closed as its duplicate).  Found by
Bugzilla REST quicksearch 2026-09-25 on `constant delete base virtual`.
**Affects:** measured 2026-09-25 -- 14.4.0, 15.3.0, 16.1.0, 16.2.0, stock
trunk `17.0.0 20260922` and this branch all accept the reproducer.
**Workaround:** none; the code is undefined, so the fix is in the user's code
**Clang:** correct -- trunk and the Clang fork reject it with "delete of
object with dynamic type 'D' through pointer to base class type 'B' with
non-virtual destructor"

## Bug Report

Already filed as PR97647, whose own reproducer is the same shape with an
implicit destructor.  Nothing to file; if anything is added there, it is the
measurement below (still reproduces on 16.2 and trunk 20260922, with
user-provided `constexpr` destructors too).

| File | Description |
|---|---|
| [`constexpr-delete-nonvirtual-dtor.cpp`](constexpr-delete-nonvirtual-dtor.cpp) | `new D`, `delete` through `B*`, in a `static_assert` |

## Symptom

`g++ -std=c++20 -fsyntax-only constexpr-delete-nonvirtual-dtor.cpp` is
silent, exit 0.  Expected: "non-constant condition for static assertion".

## Notes (ours, not for the report)

The UB audit (`expr.delete.dynamic.type.differ`) earlier recorded GCC as
rejecting this; that rejection came from the unrelated "modification of
object from outside current evaluation" path (cf. GCC-42), and went away
when that was fixed, which is what exposed this.  The replaceable
deallocation branch of `cxx_eval_call_expression` (`gcc/cp/constexpr.cc`)
checks array versus non-array and double deallocation of the heap variable,
but nothing compares the heap variable's type with the static type the
`delete` operand had when its destructor is not virtual; that comparison is
the missing check.

## Regression test

`g++.dg/contracts/cpp26/constexpr-delete-nonvirtual-dtor.C`, which replaced the watch test when the
defect was fixed here.  The Clang mirror is
`clang/test/Contracts/constexpr-delete-nonvirtual-dtor.cpp`.
