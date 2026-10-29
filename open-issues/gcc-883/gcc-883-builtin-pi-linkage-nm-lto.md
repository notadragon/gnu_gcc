# GCC-883: `gcc.dg/builtin-pi-linkage.c` ERRORs in the rhel8 Space

**Kind:** defect
**Status:** Open -- upstream test, environmental in the build Space
**Affects:** nothing in contracts.  One `dg.exp` (`check-gcc`) `dg-final`
that cannot run.
**Upstream Link:** None found (searched 2026-10-19: Bugzilla REST quicksearch
on the test name)

## Symptom

A whole-`dg.exp` run at base `1f9dc1fbeb0` (2026-10-19), in the rhel8
Space, reports:

    PASS: gcc.dg/builtin-pi-linkage.c (test for excess errors)
    ERROR: gcc.dg/builtin-pi-linkage.c: error executing dg-final:
      /usr/bin/nm: builtin-pi-linkage.o: plugin needed to handle lto object

## Why it is not ours

The test is new upstream (`3d0eda79500`, "testsuite: Check LTO linkage for
pi-based builtins", 2026-10-15) and the branch does not touch it.  It
compiles with `-O2 -flto` (no `-ffat-lto-objects`) and then inspects the
object with `nm`; the build tree's `gcc/nm` resolves to the Space's
`/usr/bin/nm`, GNU binutils 2.30 (rhel8), which does not load GCC's LTO
plugin on its own and so cannot read a slim LTO object.  The compile itself
passes.

## To retire

Re-run `dg.exp` after each rebase.  The row goes if upstream makes the test
use `gcc-nm` (or skip without a plugin-capable `nm`), or if the Space's
binutils can read LTO objects.  Reporting it upstream is the user's call.
