# GCC-29: The `__contract_assert` extension spelling ICEs in grok_contract

**Status:** Fixed here (commit `f64b7bbafac`)
**Component:** c++ / contracts
**Upstream Link:** UNKNOWN -- not yet filed
**Affects:** measured 2026-09-05 -- reproduces on stock g++ trunk with plain
`-fcontracts`

## Bug Report

```cpp
void f (int x) { __contract_assert (x > 0); }
```

```
internal compiler error: in grok_contract, at cp/contracts.cc:2102
```

`__contract_assert` is GCC's own extension spelling: `c-common.cc` maps both
it and the standard `contract_assert` to `RID_CONTASSERT`, so the two arrive
at `grok_contract` indistinguishably as far as the parser is concerned.
`grok_contract` then tested only for the standard spelling and fell off the
end of its `if`/`else` chain into the `gcc_unreachable`-style assertion.

An ICE on a spelling the compiler itself defines, reachable with nothing but
`-fcontracts`.

## Reproducer

See [`gcc-29-contract-assert-alt-spelling-ice.cpp`](gcc-29-contract-assert-alt-spelling-ice.cpp)
in this directory, which pairs the ICE with the standard spelling as a
control -- that is what places the defect in token recognition rather than in
assertion-statements generally.

## Our Fix

Recognise both spellings in `grok_contract`.

Test: `gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-alt-spelling.C`,
which also covers the spelling inside a template and inside a lambda's own
contract, since those reach `grok_contract` by different paths.

## Notes

Found by the 2026-09-05 audit comparing every plain-`-fcontracts` test in our
suite against stock trunk. It had been fixed on the branch without ever being
classified as upstream's, which is the same gap that hid
[GCC-28](gcc-28-xobj-member-in-predicate-ctor-message.md).
