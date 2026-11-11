---
id: 0140-constexpr-address-before-ctor
subject: 'c++: record the constexpr address-before-construction defect'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not a contracts bug -- the test says so in its own header -- and NOT FIXED
here; this commit only records it.  [class.cdtor]/1 makes referring to any
non-static member or base of an object with a non-trivial constructor, before
that constructor begins, undefined, with no carve-out for forming an address
rather than reading.  Such an expression is therefore not a core constant
expression, and GCC's constant evaluator accepts it in two shapes: through a
virtual base (which Clang rejects) and through a direct member with a
user-provided constructor (which Clang also accepts).  Both `static_assert`s
carry `dg-error ... { xfail *-*-* }`, so the day either shape starts being
diagnosed the line XPASSes and we are told, rather than finding out by
re-running a reproducer by hand.

Tracked as GCC-2 in `bug-reports/`, upstream as PR126357 (partial -- that
report is the call-a-member shape, not the form-an-address one) and as
llvm/llvm-project#211286.  `open-issues/README.md` records it as `deferred`.
The `fixes:` list is empty on purpose: nothing here resolves GCC-2.

Kept separate from `0120-constexpr-vbase-lifetime`, which is a mirror of
`clang/test/SemaCXX/constexpr-vbase-lifetime.cpp` and whose A3 case notes the
same divergence in passing.  That commit is about what GCC gets right for a
derived-to-virtual-base conversion; this one is the watch test for what it
gets wrong, and the two want different subjects if either is ever posted.

## Compile gap

None.  A pure test addition against upstream behaviour, with no dependency on
anything this branch adds -- no contracts, and the only requirement is the
`dg-do compile { target c++26 }` the test already carries.  The file sits
under `g++.dg/contracts/cpp26/` only because that is where the sweep that
found it was working.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-address-before-ctor.C : *
