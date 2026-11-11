---
id: 6990-contract-violation-information
subject: 'c++, contracts: -fcontract-violation-information=full|none'
depends: [1310-contract-comment-from-expression, 2200-libcontracts, 2400-p3595, 3300-p3290, 3600-p3099, 4200-p3400-core, 5000-p4299, 6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale

A contract-violation record names the file, function, line and column of
the assertion and carries its comment (the predicate's source text) and
its message, so all of that text lands in the object file.
`-fcontract-violation-information=none` keeps it out: every record the
compiler emits for a run-time check then has an empty file name, an empty
function name, line 0, column 0, an empty comment and no message, as for
a contract without one.  The default, `full`, records everything as
before.

It is one option for C, C++, Objective-C and Objective-C++, and it covers
every record the compiler builds: `build_contract_data_block_ctor` serves
preconditions, postconditions, `contract_assert`, the implicit (P3100)
contract assertions, including those whose checks the middle end inserts,
and the comment and message a P3400 label computes, which by then are
stored on the contract; `c_emit_contract_check` serves the C contract
assertions.  Each replaces the values just before they are used, so the
contract keeps its comment and message, and constant evaluation's
diagnostics keep their locations.  The kind, semantic, detection mode and
the label fields of the record are unchanged.

The library's own records follow through a predefined macro,
`__gcc_contracts_violation_information_none`: under it `<cassert>`'s
contract-checking `assert` (P3290) passes an empty comment and a
default-constructed `std::source_location`, and the P3290 convenience
macros of `<contracts.h>` pass an empty location and comment, still
evaluating their comment argument.  The P3290 manual API,
`std::contracts::handle_*_contract_violation`, defaults its location
argument to an empty `source_location ()` instead of the caller's; a
location passed explicitly is kept, as is the comment the caller passes.

The tests run the same programs under `none` and under an explicit `full`,
check the default handler's output under each, scan the assembly for the
predicate text, messages, computed strings, function and `#line` file
names, and check that constant evaluation still diagnoses at the
assertion with its message.  Others check that the macro is defined under
`none` alone in C and C++, whatever the other contracts options, that the
last option wins, and that an invalid value is rejected.

## Compile gap

None.

## Contents

- gcc/c-family/c.opt : /^fcontract-violation-information=$/
- gcc/c-family/c.opt.urls : /^fcontract-violation-information=$/
- gcc/c-family/c-cppbuiltin.cc : /__gcc_contracts_violation_information_none/
- gcc/doc/invoke.texi : /^-fcontract-violation-information=/, /^@opindex fcontract-violation-information$/
- gcc/cp/contracts.cc : @build_contract_data_block_ctor:1
- gcc/c/c-parser.cc : @c_emit_contract_check:1
- gcc/ginclude/contracts.h : @__attribute__:1
- libstdc++-v3/include/bits/assert_contract.h : @__assert_single_argument:1
- libstdc++-v3/include/std/contracts : @_GLIBCXX_END_NAMESPACE_VERSION:1
- gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-*.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-violation-information-*.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3290-api-violation-information-*.C : *
- gcc/testsuite/gcc.dg/contracts/c-violation-information-*.c : *
