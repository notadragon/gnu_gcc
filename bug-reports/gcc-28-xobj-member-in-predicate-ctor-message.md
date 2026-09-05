# GCC-28: A member named in an explicit-object member function's contract is diagnosed with a constructor/destructor message

**Status:** Fixed here (commit `a611ae28328`)
**Component:** c++ / contracts
**Upstream Link:** UNKNOWN -- not yet filed
**Affects:** measured 2026-09-05 -- reproduces on stock g++ 16.2.0 and trunk
with plain `-fcontracts`; g++ 13/14/15 do not accept the contracts syntax
these use

## Bug Report

Naming a non-static data member unqualified in a contract predicate of an
**explicit object member function** is rejected -- correctly -- but with a
message about a rule that does not apply:

```cpp
struct S { int x = 0; };
struct T : S {
  void f (this T &self) pre (x == 0);
};
```

```
error: 'S::x' 'this' required when accessing a member within a constructor
precondition or destructor postcondition contract check
```

`f` is neither a constructor nor a destructor. The expected diagnostic is the
one the function **body** already gives for the same expression, "invalid use
of non-static data member 'S::x'" -- an unqualified member means `(*this).x`,
and an explicit object member function has no `this`
([expr.prim.this]/1).

The same happens for an unqualified member function **call** in a predicate
("cannot call member function ... without object"), and in a precondition, a
postcondition and an assertion-statement alike.

The explicit `this` spelling was always correct -- "'this' is unavailable for
explicit object member functions" -- which is what made the wrong message
conspicuous: the right words were available one line away.

## Root cause

`cp_parser_late_contract_condition` sets `contract_class_ptr` to
`current_class_ptr` for a constructor precondition or a destructor
postcondition, and to `NULL_TREE` otherwise. `finish_id_expression` then asks
only whether the two are **equal**. An explicit object member function has no
`this`, so `current_class_ptr` is also null there and the comparison succeeds
against a diagnostic that has nothing to do with it. The guard was a
coincidence, not a test.

## Reproducer

See [`gcc-28-xobj-member-in-predicate-ctor-message.cpp`](gcc-28-xobj-member-in-predicate-ctor-message.cpp)
in this directory. It carries four affected shapes and four controls; the
controls matter, because two of them are what bound the defect:

* the explicit `this` spelling already produced the right message, and
* a **genuine constructor precondition** naming a member unqualified must keep
  producing the constructor message -- both compilers do, before and after.

## Our Fix

Test `contract_class_ptr` for non-nullness before comparing. The case then
falls through to the ordinary path and gets the same diagnostic the body
gives. Both guards in `finish_id_expression` have the defect and both are
fixed; the member-function one had no test coverage at all before.

Test: `gcc/testsuite/g++.dg/contracts/cpp26/deducing-this-no-this-in-predicate.C`.

## Notes

Found by a deliberate deducing-this x contracts sweep, which is also what
turned up [GCC-17](gcc-17-this-in-xobj-declaration.md) -- `this` accepted in
the trailing return type of an explicit object member function. The two are
the contracts-specific and non-contracts halves of the same sentence in
[expr.prim.this]/3, and GCC-17's "found while fixing the contracts-specific
analog" means this entry.

A diagnostic-quality bug rather than an accepts-invalid one: the program is
correctly rejected either way. That is the same class as
[GCC-14](gcc-14-contract-capture-note-garbage-location.md), which is tracked
on the same terms.
