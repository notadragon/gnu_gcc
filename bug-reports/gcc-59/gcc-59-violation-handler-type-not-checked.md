# GCC-59: The declared type of `::handle_contract_violation` is not checked

**Status:** Fixed here (commit [546b875ea518](https://github.com/notadragon/gnu_gcc/commit/546b875ea51827bf4b8d86f4a6555f2faf0f4032))
**Resolved by:** --
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-25: Bugzilla REST quicksearch
on `handle_contract_violation` returns PR83077, PR109787, PR124264, PR125163,
PR126572 and PR127414, none of which is this)
**Affects:** measured 2026-09-25 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260922` accept every case in the reproducer.  This branch rejects
all three.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] the type of a user-provided ::handle_contract_violation is not checked` |

Attachments:

| File | Description |
|---|---|
| [`violation-handler-type-not-checked.cpp`](violation-handler-type-not-checked.cpp) | Three ill-formed handler declarations, all accepted |

````
[basic.contract.handler] requires the contract-violation handler, a function
named ::handle_contract_violation, to "have a single parameter of type
'lvalue reference to const std::contracts::contract_violation' and ... return
void".  Unlike the [dcl.fct.def.replace] requirements on replacement
functions, this rule is not "no diagnostic required", but GCC does not check
it at all:

  #include <contracts>
  int handle_contract_violation (int) { return 0; }                     // accepted
  void handle_contract_violation (const volatile std::contracts::contract_violation&) {} // accepted
  void handle_contract_violation (const std::contracts::contract_violation&, int) {}     // accepted

Each is a declaration of the contract-violation handler with the wrong type.
Expected: an error for each.  Seen with 16.1.0, 16.2.0 and trunk.
````

## Our Fix

`check_handle_contract_violation` (`gcc/cp/contracts.cc`, called from
`grokfndecl`) checks the declared type of `::handle_contract_violation`: a
`void` return, a single parameter that is an lvalue reference to exactly
`const std::contracts::contract_violation` (not `const volatile`).  Each
case in the reproducer is rejected.

## Notes (ours, not for the report)

Test: `g++.dg/contracts/cpp26/violation-handler-cv-param.C` (and the
branch's other handler-type tests for cases 1 and 3).
