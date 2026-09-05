# GCC-26: A redeclaration whose parameter type is dependent escapes the postcondition const rule

**Status:** Fixed here (commit `13da4a8bb32`)
**Component:** c++ / contracts
**Upstream Link:** [PR127196](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127196)
(UNCONFIRMED, assigned to Waffl3x, keywords `accepts-invalid, c++-contracts, c++26`)
**Affects:** measured 2026-09-04 -- reproduces on stock g++ 13.4.0, 14.4.0,
15.3.0, 16.2.0 and trunk; unfixed upstream at the time of writing

## Bug Report

```cpp
template <typename T> void f (T a) post (a);   // `a` is not const
template <typename T> void f (T const a) {}
template void f<int> (int);                    // accepted; ill-formed
```

[dcl.contract.func]/7 requires the parameter a postcondition predicate
odr-uses "and the corresponding parameter on all declarations of f" to have
const type. The first declaration's `a` is `T`, which for `T = int` is not
const, so the instantiation is ill-formed. GCC accepts it.

The gap is specific to *dependent* parameter types. With concrete types every
declaration is checked, at parse time, on the declaration carrying the
contract. With a dependent one that declaration was never re-examined once
the arguments were known -- by then it no longer existed: `duplicate_decls`
merges the declarations and the **definition's** parameters are the ones that
survive, so the offending `T a` had been discarded.

Two controls in the reproducer establish that framing rather than leaving it
inferred:

* the same shape with concrete types is correctly rejected, so the check does
  run and the dependence is what defeats it; and
* both declarations non-const, dependent, is correctly rejected, so the check
  does run at instantiation -- it simply only ever sees the surviving
  parameter.

## Reproducer

See [`gcc-26-postcondition-redecl-dependent-param.C`](gcc-26-postcondition-redecl-dependent-param.C)
in this directory. The lines that were wrongly accepted before the fix are the
`Reported` and `ThreeDeclarations` namespaces; the rest are controls, four of
which must keep compiling.

## Our Fix

Record, against the surviving `FUNCTION_DECL`, any parameter of a merged-away
declaration whose dependent type is not const; substitute those types in
`tsubst_function_decl` once the arguments are known and require const.

Three details the fix depends on, each of which a simpler version got wrong:

* **Key on the function and record both sides of a merge.** Recording only
  the older declaration's parameter, keyed on the newer one, loses the middle
  declaration of three, and presumes which parameters survive -- which depends
  on whether the new declaration is a definition.
* **Substitute the recorded type; do not compare cv-qualifiers.** The two
  declarations disagree about writing `const`, but that does not settle it:
  instantiated with `T = const int` both parameters are const and the program
  is well-formed. A structural comparison rejects that; there is a test for
  the corner.
* **`uses_template_parms`, not `dependent_type_p`.** The recording runs from a
  function reached both from `duplicate_decls` and from
  `tsubst_function_decl`; in the latter `processing_template_decl` is 0, where
  `dependent_type_p` asserts on a `TEMPLATE_TYPE_PARM`.

## Notes

Also affects our Clang branch, which accepts the same reproducer; that is not
filable, since Clang's contracts implementation is not upstream. A Clang-side
fix is owed and is not yet done.

The reporter's own note on the PR was "I reckon this will need a bit of a
refactor to fix" -- most of which had already been done by the PR126897 work
(GCC-19), since the walk over the finished predicate is what makes an
instantiation-time check possible at all.

Two of the reproducer's control cases draw two diagnostics each -- the
redeclaration check and the walk over the substituted predicate both
reporting, on different lines. Measured against stock g++ 16.2: `BothNonConst`
double-reports there too, so that one is pre-existing; `OtherWayRound` gains
its second diagnostic from the GCC-19 fix rather than from this one. Neither
is a consequence of this change, and the only diagnostics this fix adds are
the two it exists for (`Reported` and `ThreeDeclarations`).
