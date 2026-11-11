---
id: 0490-optimize-attribute-alignment-watch
subject: 'testsuite: record -O1 functions keeping an optimize ("O2") function''s alignment'
depends: []
regenerates: []
fixes: []
---

## Rationale

GCC-996.  Not a contracts bug, and not fixed here; this commit only
records it.  In a -O1 compilation, a function compiled after one whose
`optimize ("O2")` attribute gives it -O2's `-falign-functions` and
`-falign-loops` keeps that alignment: -O1 sets no `-falign-*` string, and
`parse_N_M` leaves `align_functions`, `align_loops` and the others as the
previous function set them when the string is NULL.  GCC 13.4 and 14.4 do
not show it; 15.3, 16.2 and trunk do.

The watch test counts the `.p2align` directives of such a pair on x86:
only the attribute function's are expected, and the line is xfailed, so the
day it is fixed it XPASSes.

## Compile gap

None.  A test of stock behaviour, with no dependency on anything this
branch adds.

## Contents

- gcc/testsuite/gcc.dg/contracts/open-bug-optimize-attribute-alignment.c : *
