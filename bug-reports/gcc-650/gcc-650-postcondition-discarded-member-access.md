# GCC-650: a parameter named only in a discarded member access in a postcondition must be const

**Status:** Open (upstream); fixed here
**Resolved by:** [16b8ae6840ca](https://github.com/notadragon/gnu_gcc/commit/16b8ae6840cacd49f0592ad23cbc710d951effad)
**Component:** c++ (contracts)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897)
(related: the comma left operand naming the parameter itself, GCC-19, fixed
here; this member-access form is not mentioned there.  Searched 2026-10-06:
`postcondition const member access discarded`, `postcondition odr-use
discarded`.)
**Affects:** measured 2026-10-06: stock trunk (`17.0.0 20261005`) and this
branch reject lines A and B; stock also rejects C.

## Reproducer

[`gcc-650-postcondition-discarded-member-access.cpp`](gcc-650-postcondition-discarded-member-access.cpp):

    struct S { int m; };
    void mem (S s) post ((s.m, true)) {}             // error
    void mem_void (S s) post (((void) s.m, true)) {} // error

The potential results of `s.m` include `s`; as a discarded-value expression
with no lvalue-to-rvalue conversion, `s.m` does not odr-use `s`
([basic.def.odr]), so the const requirement on postcondition parameters
([dcl.contract.func]) does not apply.  GCC-19's fix handled the parameter
named directly in the left operand of a comma, not through a member
access.  Clang rejects these too (CLANG-637 in the Clang fork); the EDG
front end accepts (`sema/postcondition_discarded_params`).

## Found by

The EDG fork's M0-M4 audit, 2026-10-06.  Now the regression test
`g++.dg/contracts/cpp26/postcondition-discarded-member-access.C`.
Possibly a comment on PR126897 (the user's call).

## Fixed here

`walk_discarded_operand` now treats a member access naming a non-static data member as its object expression's potential results ([basic.def.odr]/3), unless the access is volatile.  Regression test: `g++.dg/contracts/cpp26/postcondition-discarded-member-access.C`.
