# GCC-663: a discarded member access through a function template's declarations requires the parameter to be const

**Status:** Open (upstream); fixed here
**Resolved by:** [f9d8b9086670](https://github.com/notadragon/gnu_gcc/commit/f9d8b9086670789844a270e56b2a7f02eb66ca7c)
**Component:** c++ (contracts)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-10-07: `postcondition const
template`, `postcondition must be const`, `value parameter used in a
postcondition`).  Related: [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897)
(the comma left operand naming the parameter itself, GCC-19) and GCC-650 (the
same member-access form in a non-template, fixed here).
**Affects:** measured 2026-10-07: stock trunk (`17.0.0 20260922`) and this
branch at `747de38a5a1` reject the reproducer.

## Reproducer

[`gcc-663-postcondition-discarded-member-access-template.cpp`](gcc-663-postcondition-discarded-member-access-template.cpp):

    struct S { int m; };
    template <class T> void t (const T s) post ((s.m, true));
    template <class T> void t (T s) {}
    void u () { t (S{1}); }

    error: value parameter 's' used in a postcondition must be const
      (at the definition's `T s', "In substitution of ... [with T = S]")

`s.m` is a discarded-value expression naming a non-static data member, so
its potential results include `s` and `s` is not odr-used
([basic.def.odr]/3, /5); the const requirement of [dcl.contract.func] does
not apply to it, and the definition may drop the const.  The non-template
form is GCC-650.

## Cause

In the template the predicate `(s.m, true)` is dependent: `s.m` is a
`COMPONENT_REF` built by `build_min_nt`, whose member operand is still an
`IDENTIFIER_NODE`.  `walk_discarded_operand` exempts the object of a member
access only when the member is a `FIELD_DECL`, so it walks `s` as an
odr-use and flags the pattern's parameter used-in-postcondition.
`tsubst_function_decl` then carries the flag to the instantiation through
`check_postconditions_in_redecl` and requires the definition's `T s`, now
`S s`, to be const -- before the predicate is substituted, which is the
point at which the access is known to name a data member.

## Found by

The Clang fork's M5 mirror of CLANG-646 (`postcondition-discarded-member-access.cpp`,
`re_tmpl`), recorded as GCC-662.  Clang rejected it too until CLANG-646.
Now the regression test
`g++.dg/contracts/cpp26/postcondition-discarded-member-template.C`.

## Fixed here

`walk_discarded_operand` takes the object of a member access through a
type-dependent object as a potential result, leaving the decision to the walk
over the substituted predicate; `check_postcondition_redecl_parm_types` runs
again after that walk, and the declarations merged away are recorded whether
or not the pattern marked the parameter, so a dependent access that turns out
to name a static member still requires every declaration's parameter to be
const.  That required diagnosing the object of a static member access at all,
which this branch had stopped doing (GCC-666, fixed with it).  Regression
test: `g++.dg/contracts/cpp26/postcondition-discarded-member-template.C`.
