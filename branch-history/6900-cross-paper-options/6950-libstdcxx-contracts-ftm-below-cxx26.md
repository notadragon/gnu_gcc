---
id: 6950-libstdcxx-contracts-ftm-below-cxx26
subject: 'libstdc++: testsuite: P2900, P3099, P3100, P3290, P3400, P4301: name every contracts feature-test macro below C++26'
depends: [0180-libstdcxx-contracts-cxxmin, 3300-p3290, 3600-p3099, 4200-p3400-core, 5200-p4301, 6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale

`0180-libstdcxx-contracts-cxxmin` lowered the contracts feature-test macros
from `cxxmin = 26` to `cxxmin = 20` so that `<contracts>` follows
`-fcontracts` below C++26 (GCC-50).  Nothing measured that.
`feature_test_macros.cc` names the macros, but it runs at c++26 and c++29 --
the dialects where the old guard already passed -- so it would go on passing
if every `cxxmin` were put back.

These two files are that test at the dialects the change is about: one pinned
at gnu++20, the floor, and one at gnu++23, the dialect GCC-50 was reported
against.  Each names all eight `__cpp_lib_*` macros the change un-gates and
then declares

    void handle_contract_violation (const std::contracts::contract_violation&)

which is the symptom itself -- a violation handler is declared by naming that
type, and while the header is inert the type does not exist.  C++23 is
measured rather than inferred from the C++20 and C++26 runs, because it is the
dialect in the report and in `bug-reports/verify-cases.txt`.

**The `-std=` belongs in `dg-options`, and that is not a style choice.**
`libstdc++.exp` picks a test's dialects itself unless it finds `-std=` in the
test's own options: with none, `v3-minimum-std` reads the target selector and
runs the test at that dialect and at `v3_max_std`, which for a `{ target
c++20 }` test below the default would collapse to `v3_default_std` -- 20 here,
but not a value this test should inherit.  Naming the dialect makes each file
one run at one dialect and keeps the pair honest if the default moves.

`feature_test_macros.cc` is deliberately untouched.  Its job is the c++26
surface and it still runs there; this is a second pair of files, not a
rewrite of the first.

## Compile gap

None beyond its stated `depends`, and the list is derived from the macros the
files actually name rather than copied from `5210-libstdcxx-contracts-ftm-test`.
Eight macros: `__cpp_lib_contracts` and
`__cpp_lib_replaceable_contract_violation_handler` are upstream's, present
before this branch's changes begin and needing no `depends` entry;
`__cpp_lib_contracts_message` is `3600-p3099`; `__cpp_lib_contracts_api` and
`__cpp_lib_assert_can_use_contracts` are both `3300-p3290`;
`__cpp_lib_contracts_labels` is `4200-p3400-core`; `__cpp_lib_contracts_report`
is `5200-p4301`; and `__cpp_lib_contracts_implicit` is `6000-p3100-core`.

That last one is why this is a separate entry from `5210` rather than an
extension of it.  `5210` sits in band 5200 and P3100 lands in band 6000, so
adding a `__cpp_lib_contracts_implicit` check there would have required a
forward `depends` edge.  This entry sits after all six of its dependencies.

`0180` is a dependency and not merely an ordering preference: without it every
`#ifndef` in both files fires at gnu++20 and gnu++23, which is the point of
the pair.

`-fcontracts-p3850` alone arms all of it -- each per-paper flag carries
`LangEnabledBy(C++ ObjC++,fcontracts-p3850)` in `c.opt` -- so neither file
names an individual `-fcontracts-pNNNN`.

## Contents

- libstdc++-v3/testsuite/18_support/contracts/feature_test_macros_cxx20.cc : *
- libstdc++-v3/testsuite/18_support/contracts/feature_test_macros_cxx23.cc : *
