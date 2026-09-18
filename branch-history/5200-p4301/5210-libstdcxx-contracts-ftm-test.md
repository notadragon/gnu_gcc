---
id: 5210-libstdcxx-contracts-ftm-test
subject: 'libstdc++: testsuite: name every contracts feature-test macro the branch adds'
depends: [3300-p3290, 3600-p3099, 4200-p3400-core, 5200-p4301]
regenerates: []
fixes: []
---

## Rationale

Name every contracts feature-test macro this branch adds from one place:
`__cpp_lib_contracts_message` (`3600-p3099`), `__cpp_lib_contracts_api` and
`__cpp_lib_assert_can_use_contracts` (both `3300-p3290`),
`__cpp_lib_contracts_labels` (`4200-p3400-core`), and
`__cpp_lib_contracts_report` (`5200-p4301`) -- five macros from four
commits, since P3290 is responsible for two.  Each is otherwise checked
only incidentally inside its own paper's g++.dg tests (`p3099-message-ftm.C`,
`p3290-api-ftm.C`, `p3400-label-ftm.C`, `p4301-report-ftm.C`, and their
`*-undefined` counterparts), and per the file's own comment this is the
only place all five are exercised together from the libstdc++ testsuite --
and the only place that would notice if `bits/version.h` were regenerated
without one of the matching `version.def` entries, a failure mode that is
silent (the macro simply stops being defined) rather than a build error.
It needed its own commit rather than living inside any one paper's commit
precisely because it is not about any one paper: a single compile-only
translation unit that can only exist once the last of its four dependencies
has landed.

## Compile gap

None beyond its stated `depends`, and the list is checked against what the
test actually exercises rather than assumed correct -- a test-only commit is
exactly the shape that can be sequenced ahead of the features it needs, with
nothing in the code to say so.  `feature_test_macros.cc` names six macros
total: `__cpp_lib_contracts` (the base facility, already present in
upstream master before this branch's changes begin, and needing no `depends`
entry of its own) plus the five above.  Each of the five traces to exactly
one of `depends: [3300-p3290, 3600-p3099, 4200-p3400-core, 5200-p4301]` --
`3300-p3290` alone covers two (`contracts_api`, `assert_can_use_contracts`)
-- and all four dependency bands precede this commit's own band in the
linear history, so nothing here is checked before it exists.
`-fcontracts-p3850` (this test's only compile flag) is sufficient for all
five: each per-paper flag has its own `LangEnabledBy(C++ ObjC++,fcontracts-p3850)`
entry in `c.opt`, so `-fcontracts-p3850` arms every one of `fcontracts-p3099`,
`fcontracts-p3290`, `fcontracts-p3400` and `fcontracts-p4301` directly,
without naming any of them individually on the command line.  This entry's
dependency list is therefore complete and correctly ordered; compare
`4650-reentrancy-everything`, whose Compile gap records the case where it is
not.

## Contents

- libstdc++-v3/testsuite/18_support/contracts/feature_test_macros.cc : *
