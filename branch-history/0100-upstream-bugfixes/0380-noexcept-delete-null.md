---
id: 0380-noexcept-delete-null
subject: 'c++: record that noexcept of a delete of a null pointer constant ignores the destructor'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not fixed here; this commit only records the bug.  A delete-expression
invokes the destructor, so it is potentially-throwing when the destructor
is, whatever the value of its operand.  `delete_sanity` replaces the delete
of a null pointer constant with a no-op before the destructor call is
built, so `noexcept (delete (D *) 0)` is true for a potentially-throwing
`~D`.  The watch test expects the static assertions to pass as xfailed
`dg-bogus` lines, so a fix shows as an XPASS.  Tracked as GCC-683 in
`bug-reports/` (upstream: PR65174) and as a defect in
`open-issues/README.md`.  The `fixes:` list is empty on purpose.

## Compile gap

None.  A pure test addition against upstream behaviour; it uses no
contracts and nothing this branch adds.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-noexcept-delete-null.C : *
