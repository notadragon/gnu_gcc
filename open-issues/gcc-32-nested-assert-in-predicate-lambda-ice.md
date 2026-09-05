# GCC-32: A `contract_assert` naming a capture, inside a lambda in a function contract, ICEs

**Kind:** defect
**Status:** Open
**Affects:** `-fcontracts`, C++26 and later; a `pre` or `post` whose predicate
contains a lambda that captures a parameter, where the lambda's body has a
`contract_assert` naming that capture
**Workaround:** move the nested `contract_assert` out of the lambda, or have
it name something other than a capture; both compile

## Symptom

```
internal compiler error: in expand_expr_real_1, at expr.cc:11792
```

## Trigger

```cpp
void f (int x) pre ([x] { contract_assert (x >= 0); return x > 0; } ()) { }
```

See `gcc-32-nested-assert-in-predicate-lambda-ice.C` in this directory. Four
controls bound it tightly, and each one compiles:

| shape | result |
|---|---|
| capture, nested assert names the capture, in `pre` | **ICE** |
| the same in `post` | **ICE** |
| capture, **no** nested assert | fine |
| capture, nested assert names a **global** | fine |
| the same shape in an **assertion-statement** rather than a function contract | fine |
| the same lambda in the function **body** | fine |

So it is specific to a function-contract-specifier's predicate, and to the
nested assert naming a *captured* entity.

## Why it is open

Found 2026-09-05 by the audit, not yet investigated beyond the root-cause
lead. The assertion at that line reads:

> Variables inherited from containing functions should have been lowered by
> this point.

which says the reference inside the nested assert is still the **enclosing
function's** parameter rather than the closure's captured copy -- the capture
machinery did not reach the id-expression inside the nested contract_assert.
That is the same family as GCC-10 and GCC-15, both about a contract's inner
machinery reading the wrong object.

## Notes

**Branch-only, and reachable only because we fixed
[GCC-31](../bug-reports/gcc-31-lambda-capture-in-contract-predicate.md).**
Stock g++ rejects a capturing lambda in a predicate outright, so it can never
form this construct; our fix enabled the shape and this is codegen for it
being wrong. Nothing to file upstream until GCC-31 is fixed there.

**Clang gets all six shapes right**, including running the two that ICE here;
pinned by `clang/test/Contracts/Runnable/lambda-capture-in-contract.cpp`.

Found by strengthening `contract-assert-alt-spelling.C` to cover the paths
that reach `grok_contract` indirectly -- the ICE is not about the
`__contract_assert` spelling at all (both spellings fail identically), it was
simply the first thing to put a nested assert inside a predicate lambda.
