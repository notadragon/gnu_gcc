# GCC-50: `<contracts>` is inert below C++26, so `-fcontracts` there cannot declare a violation handler

**Status:** Open (upstream); fixed here
**Resolved by:** [67a777d28425](https://github.com/notadragon/gnu_gcc/commit/67a777d28425ba0d2c92a7aa4be0d9d4ee2e09a8)
**Component:** libstdc++
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-09-18 via the Bugzilla REST API:
`product=gcc&component=libstdc++` with `contracts` in the summary returns only
PR126572 and PR127228, neither of which is this; quicksearch on
`__cpp_lib_contracts` and on `std::contracts not declared` returns nothing).
**Affects:** re-measured 2026-09-20 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260919` (`8c93f20be26`) all reproduce; 15.3.0 and earlier ship no
`<contracts>` at all.  Fixed on this branch 2026-09-20.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `libstdc++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] <contracts> is gated on __cplusplus rather than __cpp_contracts, so -fcontracts below C++26 cannot declare a violation handler` |

Attachments:

| File | Description |
|---|---|
| [`contracts-header-inert-below-cxx26.cpp`](contracts-header-inert-below-cxx26.cpp) | Rejected at `-std=c++23 -fcontracts`, accepted at `-std=c++26 -fcontracts` |

````
-fcontracts enables C++26 contract assertions below C++26.  The language half
follows the flag: at -std=c++23 -fcontracts the compiler defines
__cpp_contracts to 202502L and accepts a function carrying a precondition.
The library half does not follow it, so a violation handler cannot be
declared there.

  #include <contracts>

  // Accepted at -std=c++23 -fcontracts.
  int f (int x) pre (x > 0) { return x; }

  // Rejected at -std=c++23 -fcontracts, accepted at -std=c++26 -fcontracts.
  void handle_contract_violation (const std::contracts::contract_violation &)
  {
  }

  $ g++ -std=c++23 -fcontracts -fsyntax-only repro.cpp
  repro.cpp:3:44: error: 'contracts' in namespace 'std' does not name a type
  repro.cpp:3:74: error: expected unqualified-id before '&' token
  repro.cpp:3:73: error: expected ')' before '&' token
  repro.cpp:3:75: error: expected initializer before ')' token

  $ g++ -std=c++26 -fcontracts -fsyntax-only repro.cpp
  $

The C++26 invocation is the control: the file is otherwise valid, so the
C++23 diagnostics come from the guard described below and not from the
reproducer.

That the language half is on at C++23:

  $ echo | g++ -std=c++23 -fcontracts -x c++ -dM -E - | grep __cpp_contracts
  #define __cpp_contracts 202502L

std::contracts::contract_violation is the parameter type of a replacement
violation handler ([basic.contract.handler]), so while the type is undeclared
the default handler is the only one a program can reach.


DISCOVERY

Found 2026-09-18 while adding a throwing violation handler to an existing test
that was already compiling at -std=c++23 -fcontracts.  The handler could not
be written: declaring it names std::contracts::contract_violation, and at that
dialect the type does not exist.  The test itself was unaffected by the
absence until that point, so nothing had previously had reason to name a type
from the header there.


ANALYSIS

<contracts> wraps its whole body in #ifdef __cpp_lib_contracts.  That macro is
generated into bits/version.h from this entry in
libstdc++-v3/include/bits/version.def:

  ftms = {
    name = contracts;
    values = {
      v = 202502;
      cxxmin = 26;
      extra_cond = "__cpp_contracts >= 202502L";
    };
  };

version.tpl renders cxxmin = 26 as __cplusplus > 202302L and ANDs it with the
extra_cond:

  #if !defined(__cpp_lib_contracts)
  # if (__cplusplus >  202302L) && (__cpp_contracts >= 202502L)
  #  define __cpp_lib_contracts 202502L

At -std=c++23 -fcontracts the second conjunct holds and the first does not, so
the macro is not defined and the header expands to nothing but its include
guard.

The two conjuncts are answering different questions.  The extra_cond tracks
whether the feature is enabled, and it does real work: at
-std=c++26 -fno-contracts, __cpp_contracts is not defined and the extra_cond
alone keeps the header empty, which is correct.  The cxxmin additionally
restricts the macro to C++26 and later, while the flag that enables the
feature is honoured from C++11 -- measured on 16.2.0, `-fcontracts
-fsyntax-only` accepts `int f (int x) pre (x > 0) { return x; }` at c++11,
c++17, c++20, c++23 and c++26.

replaceable_contract_violation_handler, in the same file, carries cxxmin = 26
on both of its values blocks and is inert below C++26 for the same reason.
Those two entries are the whole of the affected surface in version.def.

A library feature whose language half is enabled before its standard by a -f
flag is written the other way a few hundred lines above, in the same file:

  name = coroutine;
  values = {
    v = 201902;
    cxxmin = 14;
    extra_cond = "__cpp_impl_coroutine";
  };

so <coroutine> is usable from C++14 when -fcoroutines is on, gated on the
language feature-test macro.

The dialect at which <contracts> itself stops compiling is C++20, not C++26.
Defining the library macros on the command line so the guard is bypassed, and
compiling the header:

  gnu++17   error: 'source_location' in namespace 'std' does not name a type
            (contracts:164; note: only available from C++20 onwards)
  gnu++20   clean
  gnu++23   clean

std::source_location is the only dependency that fails, and its own entry in
version.def carries cxxmin = 20.


VERSIONS -- all on x86_64-linux-gnu, measured 2026-09-20

  version                              -std=c++23 -fcontracts   -std=c++26 -fcontracts
  13.4.0                               no <contracts> header    no <contracts> header
  14.4.0                               no <contracts> header    no <contracts> header
  15.3.0                               no <contracts> header    no <contracts> header
  16.1.0                               4 errors                 accepted
  16.2.0                               4 errors                 accepted
  17.0.0 20260919, 8c93f20be26         4 errors                 accepted

On 13.4.0, 14.4.0 and 15.3.0 the reproducer cannot be built at all:
`fatal error: contracts: No such file or directory`.  15.3.0 also still
defines the Contracts TS value __cpp_contracts 201906L.  So the defect is as
old as the header.
````

## Reproducer

See [`contracts-header-inert-below-cxx26.cpp`](contracts-header-inert-below-cxx26.cpp)
in this directory.  It has an `#include <contracts>`, so a `.ii` should
accompany any Bugzilla attachment.

## Our Fix

`libstdc++-v3/include/bits/version.def`: `cxxmin = 20` in place of
`cxxmin = 26` on every contracts entry, leaving the `extra_cond` on each to do
the gating, and `version.h` regenerated to match.  C++20 is the floor the
header imposes on itself through `std::source_location`, per the measurement
in the analysis above.

That covers nine entries here against upstream's two, because this branch adds
seven more contracts feature-test macros (`contracts_message`,
`contracts_api`, `assert_can_use_contracts`, `contracts_implicit`,
`contracts_labels`, `contracts_report`), each gated on a language macro its
own `-f` flag predefines and each carrying the same `cxxmin = 26`.

`libstdc++-v3/include/bits/assert_contract.h` needed the same floor
separately: its body is wrapped in a hand-written
`__cplusplus > 202302L && defined(__gcc_contracts_p3290)`, which version.def
does not reach.  That header is this branch's (P3290 assert integration), not
upstream's, so it is not part of the report above.

Coverage is
[`18_support/contracts/feature_test_macros_cxx20.cc`](../../libstdc++-v3/testsuite/18_support/contracts/feature_test_macros_cxx20.cc)
and its `_cxx23` sibling: each pins its dialect with `-std=` in `dg-options`,
names all eight `__cpp_lib_*` macros the change un-gates, and declares a
violation handler, which is the symptom itself.  The existing
`feature_test_macros.cc` is unchanged and still runs at c++26 and c++29.

Kept out of the report block deliberately.  The analysis there says where the
defect is; what to do about it is upstream's call.

## Notes

The same two-line change applies to stock GCC, where only the `contracts` and
`replaceable_contract_violation_handler` entries exist.  That is a fact about
the code, not a submission: nothing here has been sent upstream, and the row
stays open because upstream still carries the defect.

`cxxmin = 20` is a floor, not a claim that C++20 is the right answer for
upstream.  The contract syntax is accepted from C++11 with `-fcontracts`; what
stops the library header lower than C++20 is `std::source_location`.  Any
upstream discussion of the floor is separate from the defect.
