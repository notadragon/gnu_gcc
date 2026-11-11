---
id: 5090-ignore-copy-facts-watch-tests
subject: 'c++, contracts: test facts from an ignored assertion reaching other TUs'
depends: [2400-p3595, 5000-p4299]
regenerates: []
fixes: []
---

## Rationale

Tests, for a fix made earlier in the series.  The evaluation semantic
of a contract assertion may differ between the copies of an inline
function in different translation units (P2900: mixed mode is not an ODR
violation).  GCC derived facts about such a function from its own TU's
copy -- nothrow in the C++ front end's `finish_function`, in
`pass_nothrow` and from the RTL; const and pure in pure-const; "stores
nothing" in mod-ref -- and compiled the callers in that TU accordingly.
When this TU ignores the assertion and the linker keeps the copy of a TU
that evaluates it, a throwing violation handler unwound through callers
compiled as if nothing could throw (a `catch` skipped and `std::terminate`
run; destructors and C `cleanup`s not run), and a handler's store to
memory was not reloaded by the caller.  This reproduces on stock GCC with
`-fcontract-evaluation-semantic=` per TU.  The refinable-body mark fixes
it: `0430-refinable-body-contracts` marks C++ functions holding a contract
assertion, `5000-p4299` C ones.

Seven multi-TU run tests check it, selecting the semantics per assertion
with a configuration file: an inline function with the front end's
nothrow (a `catch`, a destructor), the same with nothrow found only by the
middle end, a function template, the memory form, and a C99 inline
function in C compiled with `-fexceptions` (the C files compiled as C with
`-x c`), in both forms.  Tracked as GCC-896 in `bug-reports/`.

## Compile gap

None.  Pure test additions.  They need the configuration file (P3595) to
give one TU's assertion a different semantic from the other's, and the
C99 ones need C contract assertions (P4299).

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable* : *
