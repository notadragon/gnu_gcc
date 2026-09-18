---
id: 0150-this-in-xobj-declaration
subject: 'c++: record that `this` is accepted in an xobj trailing return type'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not a contracts bug -- the test says so in its own header -- and NOT FIXED
here; this commit only records it.  [expr.prim.this]/3, as amended by P0847R7,
says `this` "shall not appear within the declaration of either a static member
function or an explicit object member function of the current class".  One
sentence, two kinds of function, and GCC applies only the static half: a
trailing return type on an explicit object member function may name `this` and
is accepted.  The noexcept-specifier on the same function IS rejected, which
is what shows the rule is half-applied rather than absent.

The test is one row per shape: the accepted trailing return type carries
`dg-error ... { xfail *-*-* }` so it XPASSes when fixed; the noexcept row, the
two static controls and the two implicit-object controls are ordinary
expectations, and they are what a fix for the first row must not regress.

Tracked as GCC-17 in `bug-reports/`, upstream as PR127290 (ours), and as
CLANG-8 in the companion Clang fork -- where two rows are xfailed rather
than one, split across two files because lit's XFAIL is per file where
DejaGnu's is per line.  `open-issues/README.md` records it as `deferred`.  The
`fixes:` list is empty on purpose: nothing here resolves GCC-17.

## Compile gap

None.  A pure test addition against upstream behaviour, with no dependency on
anything this branch adds -- no contracts, and the only requirement is the
`dg-do compile { target c++23 }` the test already carries.  The file sits
under `g++.dg/contracts/cpp26/` only because the deducing-this sweep that
found it was working there.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-this-in-xobj-declaration.C : *
