---
id: 6980-caller-side-unwrapped-watch-test
subject: 'c++, contracts: watch test for a caller with no caller-side check'
depends: [0430-refinable-body-contracts, 2400-p3595]
regenerates: []
fixes: []
---

## Rationale

A watch test for a known limitation of the refinable-body mark, deferred:
whether a call is checked on the caller's side (P3595 caller-side
configuration) is decided per translation unit, and a function is marked
for the evaluations of its calls only where it calls a contract wrapper.
An inline function compiled where its calls are not checked is therefore
not marked, so its callers there take it to be nothrow, while the linker
may keep another unit's definition, which checks a call on the caller's
side and calls a violation handler that throws.  The multi-unit run test
selects the caller-side semantic per unit by location in a configuration
file and is expected to fail (`dg-xfail-run-if`), so that a fix shows up
as an XPASS.  Tracked as GCC-904 in `open-issues/README.md`; the
limitation is accepted as long as caller-side checking is configured alike
in every unit that defines the same inline function.

## Compile gap

None.  Pure test additions.  They need the configuration file's
caller-side entries (P3595) and the refinable-body mark.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-caller-side-unwrapped* : *
