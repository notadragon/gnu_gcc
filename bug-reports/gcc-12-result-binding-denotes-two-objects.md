# GCC-12: Postcondition result binding denotes two different objects within one predicate evaluation

**Status:** Fixed here (commit `20eed05e8c4`)
**Component:** c++ / contracts
**Upstream Link:** [PR112794](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112794)
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901)

## Bug Report

Within a single evaluation of a single postcondition predicate, GCC's result
binding `r` resolves to two different objects depending on whether it is
accessed directly or through a reference parameter. The caller-visible
symptom is PR112794's lost/observed `const_cast` writes depending on
spelling -- the standard requires an evaluated predicate's effects to be
faithfully observable, and a result binding that aliases inconsistently
violates that.

## Reproducer

See [`gcc-12-result-binding-denotes-two-objects.cpp`](gcc-12-result-binding-denotes-two-objects.cpp)
in this directory.

## Our Fix

A gimplification fix making the result binding denote a single, consistent
object regardless of access spelling, per the standard's requirement that an
evaluated predicate's effects be faithfully observable.

## Notes

Matches upstream PR112794; our reproducer and fix can be linked as a comment
on that report.
