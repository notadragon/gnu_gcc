---
id: 0180-libstdcxx-contracts-cxxmin
subject: 'libstdc++: P2900: let the contracts feature-test macros follow -fcontracts'
depends: []
regenerates: [libstdc++-v3/include/bits/version.h]
fixes: [gcc-50]
---

## Rationale

GCC-50.  `-fcontracts` enables contract assertions below C++26 -- measured on
16.2.0, the flag makes `int f (int x) pre (x > 0) { return x; }` compile at
c++11 through c++26 -- but `<contracts>` did not follow it there.  The header
wraps its whole body in `#ifdef __cpp_lib_contracts`, and `version.def` gave
that macro `cxxmin = 26`, which `version.tpl` renders as
`__cplusplus > 202302L` and ANDs with the entry's `extra_cond`.  At
`-std=c++23 -fcontracts` the `extra_cond` held and the `cxxmin` did not, so
the header expanded to nothing but its include guard and
`std::contracts::contract_violation` was undeclared.  Since that type is the
parameter of a replacement violation handler, no handler could be declared
below C++26 and the default one was all a program could reach.

The two conjuncts answer different questions, and only one of them was wrong.
The `extra_cond` tracks whether the feature is enabled and does real work:
at `-std=c++26 -fno-contracts`, `__cpp_contracts` is undefined and the
`extra_cond` alone keeps the header empty, which is correct.  It is the
`cxxmin` that additionally restricted the macro to a standard the flag does
not restrict itself to.  Both entries that exist in upstream master --
`contracts` and `replaceable_contract_violation_handler`, the latter on both
of its `values` blocks -- carried it.

`cxxmin = 20` in place of `cxxmin = 26` leaves the `extra_cond` to do the
gating, which is how `coroutine` is written a few hundred lines above in the
same file (`cxxmin = 14`, `extra_cond = "__cpp_impl_coroutine"`) so that
`<coroutine>` follows `-fcoroutines`.

**C++20 is measured, not chosen.**  Defining the library macros on the
command line so the guard is bypassed, and compiling the header: gnu++17
fails with `'source_location' in namespace 'std' does not name a type` at
`contracts:164`, gnu++20 and gnu++23 are clean.  `std::source_location` is the
only dependency that fails below C++20, and its own `version.def` entry
carries `cxxmin = 20`, so the floor here is the one the header already
imposes on itself.

Behaviour at C++26 is unchanged, and at every dialect without `-fcontracts`
the `extra_cond` is false and the macros stay undefined exactly as before;
what changes is only the C++20-to-C++23 range with the flag on.

## Compile gap

None.  Two `cxxmin` values in a data file and the guard lines regenerated from
them, against entries that are upstream's byte for byte at this point in the
history.  Nothing here reads anything this branch adds later.

This branch's own seven contracts entries in the same file carry the same
`cxxmin = 26` and need the same floor, but each arrives with the paper that
adds it and is changed in place there rather than here -- this entry would
otherwise have to edit lines that do not yet exist.  The same applies to
`bits/assert_contract.h`, whose hand-written `__cplusplus > 202302L` guard
`version.def` does not reach: it is P3290's file and its floor is lowered in
`3300-p3290`.

The test that would exercise this cannot live here either.  It names
feature-test macros from P3099, P3290, P3400, P3100 and P4301, none of which
exist yet at this point in the history, so it arrives as
`6950-libstdcxx-contracts-ftm-below-cxx26` once all of them have landed.

## Contents

Both selectors key on a **removed** line, which is what separates this entry
from the branch's own contracts entries in the same two files.  Upstream's
`contracts` and `replaceable_contract_violation_handler` are the only entries
that existed before this branch, so they are the only ones with a
`cxxmin = 26` to remove; every other contracts entry arrives as added lines
already carrying `cxxmin = 20`.  There are exactly three removed lines in each
file's whole fork diff, and they are these.  A selector naming the block
(`/name = contracts;/`) would match nothing -- a regex selector is tested
against a segment's added and removed lines, never its context.

- libstdc++-v3/include/bits/version.def : /cxxmin = 26;/, /-fcontracts enables contract assertions as an extension/
- libstdc++-v3/include/bits/version.h : /__cplusplus >  202302L/
