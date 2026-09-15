# GCC-39: Explicit instantiation of a variadic template rejects a cv-qualified pack argument

**Status:** Open (upstream); not fixed here and not a contracts bug
**Component:** c++ / templates
**Upstream Link:** [PR126797](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126797)
-- filed 2026-08-11 by a third party, UNCONFIRMED.  The testsuite comment this
writeup came from said "PR unfiled" and "worth filing"; it was already filed,
found by searching Bugzilla rather than trusting that note.

**Affects:** stock trunk (17.0.0 20260905), gcc 13.4.0, and this branch alike.
Clang accepts.  Not a regression -- the PR reports it back to the first CE
version supporting C++11.

## Bug Report

An explicit-instantiation declaration of a variadic function template is
rejected when a template argument bound to the pack is cv-qualified:

```c++
template <typename... Ts> void f (Ts...) {}
template void f<const int> (const int);   // error: does not match
template <typename T> void g (T) {}
template void g<const int> (const int);   // OK for non-variadic g
```

Measured 2026-09-09, `-std=c++11 -fsyntax-only`:

| | verdict |
|---|---|
| stock trunk | REJECTS (`template-id 'f<const int>' for 'void f(int)'`) |
| gcc 13.4.0 | REJECTS |
| this branch | REJECTS |
| clang trunk | accepts |

This is **not a contracts bug**.  It was found while testing C++26 contracts
but reproduces in plain C++11 with no contract and no pack indexing, which is
why it is recorded here rather than in the contracts suite.

## Analysis

Carried over verbatim from the testsuite comment it used to live in,
including its closing recommendation to file the bug -- which is stale, as
the header above records: it was already filed, as PR126797.  Line numbers
are as of the 2026-09 trunk.

INVESTIGATION SUMMARY
-------------------------------------------------------------------------

Symptom
  template <typename... Ts> void f (Ts...) {}
  template void f<const int> (const int);   // error: does not match
  template void g<const int> (const int);   // OK for non-variadic g

The trigger is precisely: an explicit-instantiation *declaration* of a
variadic template where a template argument bound to the pack is cv-qualified
(top-level const on the by-value element, or on a pointer element such as
'int* const').  It is independent of the written parameter list (fails even
when written with the adjusted type), of any contract, and of pack indexing.
It does NOT affect a *call* with explicit template arguments
(f<const int>(0) is fine), which is why deduction from a call works.

Root cause (gcc/cp/pt.cc)
  determine_specialization -> get_bindings -> fn_type_unification matches the
  declared explicit instantiation against the primary template.  get_bindings
  builds the argument types from the declared *function type*, whose by-value
  parameters already had their top-level cv-qualifiers dropped
  ([dcl.fct.type]/5), so they are e.g. {int}.  fn_type_unification then
  substitutes the explicit template arguments into the template's parameter
  types and compares with DEDUCE_EXACT.

  For a non-pack parameter that is fully specified, 'incomplete' stays false,
  so the function type is substituted completely and tsubst_arg_types
  (pt.cc:~16851) applies cv_unqualified (type_decays_to (...)) to each
  parameter -- the element becomes {int} and matches.

  For a parameter pack given explicit arguments, fn_type_unification marks the
  pack ARGUMENT_PACK_INCOMPLETE_P and sets 'incomplete = true'
  (pt.cc:~24495-24507) even when the pack is fully specified.  That forces the
  *partial* substitution path (tf_partial, ++processing_template_decl), so the
  pack stays a pack-expansion and the explicit element (e.g. 'const int') is
  carried into the DEDUCE_EXACT comparison without the by-value parameter
  adjustment.  maybe_adjust_types_for_deduction (pt.cc:~24808-24817) performs
  no top-level cv stripping for DEDUCE_EXACT, so 'const int' is compared
  against the adjusted 'int' and unification fails -> "does not match".

  (The authoritative same_type_p check at pt.cc:~24692 compares the *fully*
  substituted candidate type, which is correctly {int}, and would pass -- but
  it is never reached because the intermediate unification fails first.)

Fix options and risk
  A. Strip top-level cv for by-value parameters under DEDUCE_EXACT (in
     maybe_adjust_types_for_deduction / unify).  DEDUCE_EXACT is used in ~16
     places including partial ordering (more_specialized_fn), conversion-
     operator overload resolution and redeclaration matching, so changing its
     exactness risks flipping partial-ordering results.  Risk: HIGH.
  B. Treat a fully-specified pack as complete (do not set 'incomplete').
     Determining "fully specified" is subtle (a pack can be part-explicit,
     part-deduced) and this alters deduction flow broadly.  Risk: HIGH.
  C. Fall back in determine_specialization: when get_bindings returns NULL,
     substitute the explicit args fully into the candidate and compare the
     resulting (cv-adjusted) function type with compparms/same_type_p.  Scoped
     to specialization / explicit-instantiation / template-friend matching;
     general deduction and partial ordering are untouched.  Risk: LOW-MEDIUM.

  Recommendation: this is a core template-deduction defect and is really an
  upstream GCC bug (worth filing); if fixed on-branch, Option C is the
  lowest-risk approach and must be gated on a full 'make check-c++', not just
  the contracts tests.

## Where the test is

`gcc/testsuite/g++.dg/template/explicit-inst-pack-cv.C`, as two `dg-bogus`
xfails plus two controls.  It XPASSes when upstream fixes this, which is the
signal to retire this row.
