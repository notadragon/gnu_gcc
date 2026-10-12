# GCC-618: a const local's constant-initialization attempt checks contracts, rejecting a well-formed program

**Status:** Open (upstream); fixed here
**Resolved by:** [7890267482b8](https://github.com/notadragon/gnu_gcc/commit/7890267482b822a72b6828da9d307570d9360f00)
**Component:** c++ / constexpr
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-09-29).  Bugzilla quicksearch on
`contract condition is not constant` returns PR125459 (our GCC-3, a predicate
re-invoking a constexpr function, fixed on trunk by `21a88667c67`) and
PR125587 (our GCC-56, a postcondition reading its result name); neither is
this.  `contract predicate is false in constant expression` returns nothing.
**Affects:** measured 2026-09-29 against stock trunk (`17.0.0 20260922`,
`~/repos/gnu_gcc-trunk-install`), 16.1.0, 16.2.0 and this branch: all
reject both forms below identically.  Clang (this fork's companion) accepts
both.
**Found in:** `mpusz/mp-units`, `src/core/include/mp-units/framework/scaling.h`
(`get_one` called from `div_round`).  mp-units disables the precondition on
GCC <= 16; a compiler reporting 17 re-enables it, and
`test/static/{quantity_test,fixed_point_test,quantity_point_test}.cpp` fail
with this diagnostic.

## Reproducers

[`gcc-618-const-local-init-checks-contracts.cpp`](gcc-618-const-local-init-checks-contracts.cpp),
at `-std=c++26 -fcontracts`:

    constexpr int get_one(const int& value) pre(value != 0)
    { return value / value; }

    constexpr int wrapper(int divisor)
    {
      const int step = get_one(divisor);
      return step;
    }

    static_assert(wrapper(8) == 1);

    $ g++ -std=c++26 -fcontracts -c repro.cpp
    repro.cpp: In function 'constexpr int wrapper(int)':
    repro.cpp:1:41: error: contract condition is not constant
    repro.cpp: At global scope:
    repro.cpp:10:26: error: non-constant condition for static assertion
    repro.cpp:10:22: error: 'constexpr int wrapper(int)' called in a constant expression

[`gcc-618-runtime-only-violation.cpp`](gcc-618-runtime-only-violation.cpp)
is the same defect with a precondition that is constant and false, in a
function never used in a constant expression:

    int runtime_only()
    {
      const int step = get_one(0);   // get_one as above
      return step;
    }

    error: contract predicate is false in constant expression

[`gcc-618-mp-units-shape.cpp`](gcc-618-mp-units-shape.cpp) is the templated
`div_round`/`get_one` shape from mp-units.

## Why both are well-formed

`step` is a const integral variable, so GCC asks whether it is
constant-initialized.  [expr.const] answers that by evaluating the
initializer *with every contract assertion having the ignore evaluation
semantic*; only a variable that passes is then evaluated with its contracts
checked, and only that second evaluation can make the program ill-formed
([basic.contract.eval], [intro.compliance.general]).

With contracts ignored, neither initializer is constant: `value / value`
reads `divisor`, a parameter of a function not being constant-evaluated, or
divides by zero.  So `step` is not constant-initialized, it is initialized at
run time, and no contract is checked at compile time.  In the first form
`wrapper(8)` is later constant-evaluated for the `static_assert`, where the
precondition `8 != 0` holds.  In the second the violation is detected at run
time.

## What the defect is, and is not

All measured on stock trunk and this branch:

| probe | result | what it establishes |
|---|---|---|
| the first reproducer, `static_assert` removed | `not constant` only | the error comes from defining `wrapper`, not from the `static_assert`, whose errors are follow-on |
| `int step` instead of `const int step` | clean | only a variable that might be constant-initialized triggers it |
| `return get_one(divisor);`, no local | clean | no constant-initialization attempt, no error |
| namespace-scope `constexpr int step = get_one(8);` | clean | the argument is constant there; the attempt succeeds and the precondition holds |
| `get_one(int value)`, by value | clean | copying the unknown argument fails before the precondition is reached; a reference binds, so evaluation reaches the precondition |
| `pre(true)` | clean | an ignored-or-satisfied predicate records nothing |
| `contract_assert(value != 0);` in the body instead of `pre` | `not constant` | not specific to function contract specifiers |
| templates, `auto` return, a precondition on `wrapper` too | no effect | incidental to the mp-units shape |

The fourteen-case isolation that produced most of this is preserved with the
original report in `notadragon_wg21/tmp/gcc-precondition-local-var-repro/`.

## Cause (read from the source)

`store_init_value` evaluates a potentially-constant variable's initializer
with `maybe_constant_init (value, decl, /*manifestly_const_eval=*/true)`.
The evaluator checks each contract as it reaches it and records a violation
-- non-constant predicate, or false -- in the global context without making
the evaluation itself non-constant.  At the end,
`cxx_eval_outermost_constant_expr` calls `check_for_failed_contracts` even
when the evaluation was non-constant (`non_constant_p` with
`allow_non_constant`), and since the context is manifestly constant-evaluated
that reports the recorded violation as a hard error.  That function's own
comment says it runs after "a successful constant evaluation".

A violation found during an evaluation that fails anyway is exactly what the
ignore-first rule discards.  An evaluation that succeeds is unaffected: its
recorded violations are still diagnosed.

## Where a constant is required

The same final check runs when a constant is required (a `constexpr` or
`constinit` variable, a `static_assert`, a template argument), and there the
program is ill-formed either way, so it is not a rejects-valid: it is the
wrong diagnostic.  With `constexpr int k = f(g);` where `f`'s body reads the
non-constant `g`, GCC reports the recorded contract problem ("contract
predicate is false in constant expression", or a warning under observe) and,
under enforce, nothing about `g` -- the actual reason `k` is not constant.
The fix is the same: a contract recorded during an evaluation that does not
complete is not reported.

## Found again

The `notional` group of the contract test matrix (2026-09-29) hits this in 80
cells: every context that decides constant initialization by notional
evaluation, and every context that requires a constant, whenever the body
stops being constant after a contract problem was recorded.  The intended
behaviour those cells encode is DECISIONS.md M7 in the notadragon repository.
