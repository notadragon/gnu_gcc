# GCC-1006: a contract predicate that throws during constant evaluation ICEs

**Status:** Open (upstream); fixed here
**Resolved by:** [befd1f2648f5](https://github.com/notadragon/gnu_gcc/commit/befd1f2648f555a09cfc10a1ffea2899da990c42)
(`discard_contract_predicate_exception`, and the exception arm of the
contract-statement case of `cxx_eval_constant_expression`)
**Component:** c++ (constexpr.cc, contract statements in constant evaluation)
**Keywords (ours -- upstream sets its own):** `ice-on-invalid-code`
**Upstream Link:**
[PR128543](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128543) -- filed
2026-11-07 by a third party (Ivan Lazaric), UNCONFIRMED, keyword
`c++-contracts`; found by the 2026-11-10 Bugzilla sweep (rebase runbook
Phase 5.5).  Its reproducer needs `-freflection`; the reduction below does
not.
**Affects:** measured 2026-11-10 -- stock trunk `17.0.0 20261110`
(`33ed38d4ee09`), 16.1.0 and 16.2.0 segfault under the `enforce`,
`observe` and `quick_enforce` semantics (`ignore` never evaluates the
predicate, so compiles clean).  This branch, rebased onto `33ed38d4ee09`,
segfaulted the same way until the fix (2026-11-10): both shapes now get
"contract predicate exited via an exception of type 'int' in constant
expression", an error under `enforce` and `quick_enforce` and a warning
under `observe`.
**Verify cases:** `GCC-1006a` (the reduction, compile,
`-std=c++26 -fcontracts`), `GCC-1006b` (PR128543's reproducer, compile,
`-std=c++26 -freflection`).
**Tests (this branch):**
`g++.dg/contracts/cpp26/contract-constexpr-throwing-predicate*.C`.

## Reproducer

[`gcc-1006-throwing-predicate-constexpr-ice.cpp`](gcc-1006-throwing-predicate-constexpr-ice.cpp):

```cpp
constexpr bool g () { contract_assert ((throw 1, true)); return true; }
static_assert (g ());
```

```
internal compiler error: Segmentation fault
```

[`gcc-1006-throwing-predicate-reflection.cpp`](gcc-1006-throwing-predicate-reflection.cpp)
is PR128543's program: `is_base_of_type (^^int, {})` throws
`std::meta::exception` for the null reflection, from a `contract_assert`
in a `consteval` block.

## Where it breaks

The `ASSERTION_STMT` / `PRECONDITION_STMT` / `POSTCONDITION_STMT` case of
`cxx_eval_constant_expression` (gcc/cp/constexpr.cc) evaluates the
predicate quietly with its own `jmp_target`, then tests only
`ctrct_non_const_p` before `integer_zerop (eval)`.  A predicate that exits
via an exception is not non-constant: it leaves `eval` null and the thrown
object in `jmp_target`, so `integer_zerop` dereferences null (the backtrace
in PR128543: `integer_zerop` <- `cxx_eval_constant_expression`).  The case
never looks at `jmp_target`, so the throw is neither treated as a
violation nor cleaned up.

## Boundary (stock trunk and this branch, 2026-11-10)

* `contract_assert ((throw 1, true))`, and `contract_assert (thr (true) &&
  true)` with a constexpr `thr` that throws: ICE.
* `contract_assert (thr ())`, the throwing call alone, or a wrapper
  function around the throwing metafunction: no ICE, but a misleading
  diagnostic -- "`g()` is not a constant expression because allocated
  storage has not been deallocated" -- the exception object the predicate
  allocated, which nothing released.  The same unhandled `jmp_target`; a
  fix should give both shapes the violation diagnostic.
* A throwing metafunction outside a contract (`bool b = is_base_of_type
  (^^int, {});` in a `consteval` block) is diagnosed properly ("uncaught
  exception of type `std::meta::exception`"), so the defect is the
  contract case's.

## Fix

Fixed here, not upstream.  On this branch, after the quiet evaluation, a
throw in `jmp_target` is a contract violation ([basic.contract.eval]: the
evaluation of the predicate exits via an exception), recorded like a false
predicate and reported with the exception's type -- and its `what ()`
string for a class derived from `std::exception`, as an uncaught
exception is.  Before the predicate's rollback, every heap object the
predicate allocated and left live (the exception object and whatever it
owns) is freed and the exception leaves the uncaught count, so the
allocation check no longer fires.  A null value without a throw is treated
as not constant.  Upstream's case has a different shape (one recorded
violation, no rollback), so the change does not apply there as it stands.
Nothing is filed or commented upstream by Claude.
