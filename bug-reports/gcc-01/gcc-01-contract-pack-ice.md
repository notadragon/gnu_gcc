# GCC-1: Contract on a function with a parameter pack breaks the redeclaration check

**Status:** Fixed here (commit [c249d3afe881](https://github.com/notadragon/gnu_gcc/commit/c249d3afe881d49cde65e5d33ccb6e010e3f4614))
**Resolved by:** `1100-postcondition-const-across-redeclarations`
**Component:** c++ / contracts
**Upstream Link:** [PR124395](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124395) (also duplicates PR126837, PR126836, PR127343)
**Affects:** stock g++ 16.2.0, trunk; originally confirmed via public
Compiler Explorer, 2026-08-20

## Bug Report

A contract specifier on a function with a variadic parameter pack breaks
`check_postconditions_in_redecl`'s lockstep walk of the template's and
instantiation's parameter lists. Depending on the pack shape this either
ICEs, or silently rejects a well-formed program with a diagnostic naming the
wrong parameter. Three distinct pack shapes are covered by the reproducers
below: an empty pack, a pack in the tail position that ICEs, and a pack in
the tail position that instead produces a misattributed diagnostic.

## Reproducer

See [`gcc-01a-contract-pack-empty-ice.cpp`](gcc-01a-contract-pack-empty-ice.cpp),
[`gcc-01b-contract-pack-tail-ice.cpp`](gcc-01b-contract-pack-tail-ice.cpp), and
[`gcc-01c-contract-pack-tail-misattributed.cpp`](gcc-01c-contract-pack-tail-misattributed.cpp)
in this directory.

## Our Fix

`gcc/cp/contracts.cc`: align the parameter run before the first pack from
the front, and the run after the last pack from the back, instead of a
lockstep walk, so the check is correct regardless of what the pack expands
to.

## Notes

This bug matches an existing upstream report (PR124395) and duplicates three
others (PR126837, PR126836, PR127343), so it is a good candidate for a
comment linking our fix and reproducers rather than a fresh filing.

**It keeps being rediscovered.** PR124395 has itself absorbed PR126345 and
PR124708 as duplicates, and PR127343 arrived on 2026-09-11 as a fresh
UNCONFIRMED report of the same segfault -- six reports of one walk, with
PR124395 confirmed and assigned since 2026-08-15 and still unfixed. That is
the argument for spending the effort to submit the patch rather than only
commenting it.

PR127343 spells the reproducer as an abbreviated function template --
`void foo(auto... args) pre(false) {}` -- where the pack is invented rather
than written. Nothing in the walk distinguishes an invented template
parameter pack from a declared one, and re-measured on 2026-09-11 it behaves
exactly as symptom (a): stock g++ 16.2.0 segfaults, this branch compiles it
clean. That spelling is now pinned down in the test as well, since it is the
one reporters keep reaching for.

## Pending upstream comment (PR124395)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR124395, PR126837,
PR126836 or PR127343.  Every claim re-measured 2026-09-25 against stock
`17.0.0 20260922` (the Compiler Explorer nightly) and 16.2.0.  Attach the
`check_postconditions_in_redecl` part of
[c249d3afe881](https://github.com/notadragon/gnu_gcc/commit/c249d3afe881d49cde65e5d33ccb6e010e3f4614) and its test
`g++.dg/contracts/cpp26/dcl.contract.res.p1-pack-empty.C`; that entry also
carries the GCC-26 (PR127196) fix and depends on
[16b8ae6840ca](https://github.com/notadragon/gnu_gcc/commit/16b8ae6840cacd49f0592ad23cbc710d951effad), so it needs cutting down before it can go
to gcc-patches on its own.*

~~~text
We have a tested fix for this, and the same defect is behind PR126837,
PR126836 and PR127343.

check_postconditions_in_redecl carries the "odr-used in a postcondition"
property from the template's parameters to the instantiation's by walking
the two parameter lists in lockstep.  A function parameter pack breaks that
correspondence: it is one parameter in the pattern and N in the
instantiation.  Which symptom appears depends on the shape:

- The pack expands to nothing: the walk runs off the end of the shorter
  list and segfaults.  That is this PR, and PR127343, which is the same
  thing spelled as an abbreviated function template (void foo(auto...
  args)).  Any contract specifier starts the walk; pre(true) is enough.

- A parameter written after a pack that expands to nothing: the same
  segfault one step earlier, in set_parm_used_in_post:

    template <class... A>
    int f (A... a, const int y) post (r : r > y) { return y; }
    void g () { f<> (3); }

- The pack expands to one or more elements: the walk pairs the wrong
  parameters, and a well-formed program is rejected with "value parameter
  'args#N' used in a postcondition must be const", naming a pack element
  the postcondition does not use (PR126837 for a parameter after the pack,
  PR126836 for a pack index).

The fix aligns the run of parameters before the first pack from the front
and the run after the last pack from the back -- those correspond however
the packs expand -- and skips the packs themselves, whose elements are
checked when the substituted predicate is.  The only parameter the walk
then leaves unchecked is one written between two packs, which needs a
second function parameter pack, one that can never be deduced.

The reproducers from this PR, PR126837, PR126836 and PR127343 all fail on
16.2.0 and on trunk (17.0.0 20260922), and all compile with the patch.
~~~

## Pending upstream comment (PR126837)

*Same status and checks as the PR124395 comment above.*

~~~text
This is the same defect as PR124395: check_postconditions_in_redecl walks
the pattern's and the instantiation's parameter lists in lockstep, so a
parameter after a pack is paired with a pack element, and the "must be
const" check blames args#1 for the use of a.  A patch aligning the
parameters after the last pack from the back is attached to PR124395; with
it this testcase compiles.  Suggest marking this a duplicate of PR124395.
~~~

## Pending upstream comment (PR126836)

*Same status and checks as the PR124395 comment above.  The second error
stock gives here (at `args...[1]` itself) is not the lockstep walk; on this
branch it went with the change that decides postcondition odr-use on the
substituted predicate ([16b8ae6840ca](https://github.com/notadragon/gnu_gcc/commit/16b8ae6840cacd49f0592ad23cbc710d951effad)), so the GCC-1 patch
alone clears only the first.*

~~~text
The first error here ("value parameter 'args#0' used in a postcondition
must be const") is the same defect as PR124395: the redeclaration check
walks the pattern's and the instantiation's parameter lists in lockstep,
so the pattern's pack, which the postcondition names, is paired with
args#0 alone.  The patch on PR124395 skips packs in that walk and removes
this error.

The second error, at args...[1] itself, is separate: it is reported even
though the element that index selects is const.  Our branch accepts the
testcase, but that half is fixed by a different change (deciding which
parameters a postcondition odr-uses after substitution has resolved the
index), not by the PR124395 patch.
~~~

## Pending upstream comment (PR127343)

*Same status and checks as the PR124395 comment above.*

~~~text
Same defect as PR124395, spelled as an abbreviated function template: the
invented template parameter pack is a function parameter pack like any
other, and when it expands to nothing check_postconditions_in_redecl runs
off the end of the instantiation's parameter list.  Still segfaults on
17.0.0 20260922; the patch on PR124395 fixes it.  Suggest marking this a
duplicate of PR124395.
~~~
