---
id: 0300-typeid-polymorphic-capture
subject: 'c++: record that a local in typeid of a polymorphic glvalue in a lambda is not captured'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not fixed here; this commit only records the bug.  `typeid` of a glvalue
of polymorphic class type evaluates its operand, so a local variable or
parameter named there is odr-used and a lambda with no capture-default
must capture it; and the lambda capture rule decides capture as though
enclosing `typeid` expressions were ignored.  GCC accepts the lambda
without capturing the variable.  The watch test expects the missing
diagnostic as an xfailed `dg-error`, so a fix shows as an XPASS.  Tracked
as GCC-644 in `bug-reports/` (upstream: none found) and as a defect in
`open-issues/README.md`.  The `fixes:` list is empty on purpose.

## Compile gap

None.  A pure test addition against upstream behaviour; it uses no
contracts and nothing this branch adds.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-typeid-polymorphic-capture.C : *
