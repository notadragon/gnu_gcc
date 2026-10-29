# GCC-607: a constant-evaluated call through a caller-side contract wrapper is rejected

**Kind:** defect (rejects-valid)
**Status:** Open (upstream); fixed here
**Resolved by:** [2bcd6b6193ba](https://github.com/notadragon/gnu_gcc/commit/2bcd6b6193ba24de51cd876776bb99ffb998d89f) (the wrapper inherits `constexpr`),
[70ce4a4c4235](https://github.com/notadragon/gnu_gcc/commit/70ce4a4c4235c16d5f7ce47b1b3f033776068563) (its body is materialized when constant evaluation needs it)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-09-30: Bugzilla REST quicksearch
on `contract_wrapper`, `client-check constexpr`, `contracts-client-check`)
**Affects:** measured 2026-09-30 -- stock trunk `17.0.0 20260922` rejects the
reproducer under `-fcontracts-client-check=pre` and `=all`; accepts it under
`=none`.  This branch accepts it in every mode.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `17.0` |
| Severity | `normal` |
| Summary | `[c++26][contracts] -fcontracts-client-check makes a constexpr call with a precondition non-constant` |

Attachments:

| File | Description |
|---|---|
| [`constexpr-caller-side-wrapper.cpp`](constexpr-caller-side-wrapper.cpp) | A constexpr function with a precondition, used in a static_assert |

````
With caller-side contract checking enabled, a call to a constexpr function
with a precondition is no longer a constant expression.

```
constexpr int f (int x) pre (x > 0) { return x; }
static_assert (f (1) == 1);
```

```
$ g++ -std=c++26 -fcontracts -fcontracts-client-check=pre -c t.cpp
t.cpp:2:22: error: non-constant condition for static assertion
t.cpp:2:18: error: call to non-'constexpr' function 'int f.contract_wrapper(int)'
```

The call is redirected to a caller-side wrapper that checks the
precondition, and the wrapper is not declared constexpr, so constant
evaluation of the call fails.  Without -fcontracts-client-check (or with
=none) the same program is accepted.  Expected: accepted in every mode; the
wrapper should be constexpr (or consteval) when the function is, and usable
in constant evaluation.
````

## Our Fix

`gcc/cp/contracts.cc`: the caller-side wrapper copies the callee's
`DECL_DECLARED_CONSTEXPR_P` (and immediate-function flag), and the constant
evaluator materializes a constexpr wrapper's body the first time it needs it
(the wrapper is otherwise defined only at the end of the translation unit).
