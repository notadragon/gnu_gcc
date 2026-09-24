# GCC-56: a postcondition that reads its result name is "not constant" during constant evaluation

**Status:** Open (upstream); fixed here
**Resolved by:** [b9616010f2c7](https://github.com/notadragon/gnu_gcc/commit/b9616010f2c7bbec799bb12be0973e4e566e260b)
**Component:** c++ / constexpr
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** [PR125587](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125587)
-- filed 2026-06-03 by a user, confirmed by a GCC developer (waffl3x)
2026-08-26, still `NEW` as of 2026-09-22.  **Filed by someone else, so there
is nothing for us to file**; what is owed upstream, if anything, is a patch.
**Affects:** measured 2026-09-22 against stock trunk `7aa4b1c5054`
(`17.0.0 20260922`), the `gcc-trunk-20260922` nightly, 16.2.0 and 15.3.0.
Trunk and the nightly reproduce both forms below; 16.2.0 reproduces the
variable-initializer form only; 15.3.0 has no contract syntax at all.  Fixed
on this branch.

## Reproducer

[`gcc-56-postcondition-result-not-bound-in-constexpr.cpp`](gcc-56-postcondition-result-not-bound-in-constexpr.cpp),
at `-std=c++26 -fcontracts`.  No library header is needed.

    constexpr int f (int i) post (res : res > 0) { return i; }
    static constexpr int v = f (42);

    $ g++ -std=c++26 -fcontracts -fsyntax-only repro.cpp
    repro.cpp:1:25: error: contract condition is not constant

That is upstream's own confirmed reduction, from PR125587 comment 1.  The
minimal form drops the parameter and the library entirely:

    constexpr int g () post (r : r == 1) { return 1; }
    static_assert (g () == 1);

### What the defect is, and is not

Three controls localise it.  All were measured; none is inferred.

| probe | stock trunk | what it establishes |
|---|---|---|
| `post (res : res > 0)`, true predicate | `not constant` | the baseline |
| `post (res : res > 99)`, **false** predicate | `not constant` | the predicate is **never evaluated** -- an evaluated-and-false predicate produces the *other* diagnostic, `contract predicate is false in constant expression` |
| `post (r : r == r)`, trivially true | `not constant` | the verdict does not depend on any value |
| `post (res : sizeof (res) > 0)` | **accepted** | naming the result is fine; *reading its value* is what fails |
| `pre (i == 1)` reading a parameter | **accepted** | parameters are bound; this is specific to the result name |

The false-predicate row is the load-bearing one.  `check_for_failed_contracts`
(`gcc/cp/constexpr.cc`) emits one of two messages, and `not constant` is the
branch taken when `contract_condition_non_const` was set.  Getting that
message rather than `predicate is false` for a predicate that is plainly
false proves the evaluation never ran.

## Analysis

In `cxx_eval_constant_expression`'s `POSTCONDITION_STMT` case there are two
places that set `contract_condition_non_const`:

* a **static** guard, `if (!potential_rvalue_constant_expression (cond))`,
  which runs first; and
* a check after actually evaluating the predicate under a
  `modifiable_tracker`.

This defect is the **first** one.  It fires before `modifiable_tracker` is
constructed, so the predicate is never evaluated and the value map is never
consulted.

The cause is an absent binding.  A postcondition's result-name-introducer does
not name a function parameter -- `res` is a synthetic variable the front end
builds for the predicate.  The constant evaluator binds parameters by walking
the call's argument list, so nothing ever binds `res`, and
`potential_rvalue_constant_expression` therefore rejects a predicate that
reads it.  `sizeof (res)` is accepted because an unevaluated operand needs no
binding.

That also explains the version split.  16.2.0 reaches the guard only in the
variable-initializer form; in the `static_assert` form it accepts the code
*and accepts a false postcondition too*, so it was never evaluating the
predicate there either.  No release has ever evaluated a result-naming
postcondition correctly; trunk merely diagnoses in more contexts than 16.2.0
did.

## Our Fix

[b9616010f2c7](https://github.com/notadragon/gnu_gcc/commit/b9616010f2c7bbec799bb12be0973e4e566e260b) binds the result name before the guard runs:
`constexpr_call` gains a `result_decl` field holding the remapped
`RESULT_DECL` of the body copy, `cxx_eval_call_expression` sets it, and the
`POSTCONDITION_STMT` case copies that value into the map under the
postcondition's own result variable.

This branch is measurably *correct*, not merely quiet: on a false
postcondition it reports `contract predicate is false in constant
expression`, in both the `static_assert` and variable-initializer forms.  A
fix that made the reproducer compile without that behaviour would be passing
vacuously.

## Notes

**This is not PR125459, despite an identical diagnostic string.**  Until
2026-09-22 this repository recorded the two as one defect: the GCC-3 writeup
said "PR125459 (also PR125587, same defect)" and its reproducer's header
carried both numbers.  The 2026-09-22 rebase disproved that.  Upstream took
our fix for PR125459 as `21a88667c67`, and afterwards the GCC-3 reproducer
still failed -- on exactly one of its four cases, the postcondition one.
Removing that case left the file clean on stock.  So PR125459 is fixed and
PR125587 is not; they share a message emitted from a single function, but
reach it from different code paths several hundred lines apart -- PR125459
through the post-evaluation `modifiable_tracker` check, this one through the
static guard above it.

`gcc-42`'s writeup already warned that this diagnostic is shared by unrelated
defects ("This is not PR125459/PR125587.  Those share the diagnostic but have
a different cause").  The lesson missed was that the same caution applies
*between* PR125459 and PR125587.  A shared diagnostic string is not evidence
of a shared cause, and two PRs filed independently against the same message
should be assumed distinct until one fix is measured to close both.
