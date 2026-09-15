// Contract reporting state must not escape an evaluation whose result is
// discarded.
//
// Two evaluations during constant evaluation are speculative: the operand of
// an [[assume]], which is not evaluated at all ([dcl.attr.assume]), and the
// side-effect-free trial of a contract predicate, which is thrown away and
// redone when the predicate turns out to modify the enclosing evaluation.
// Both already discard their *evaluation* failure -- each passes a local
// non_constant_p -- but a contract assertion reached along the way records
// itself on the shared constexpr_global_ctx, and that used to escape.
//
// Rejecting was the visible consequence in the first case and a duplicated
// diagnostic in the second.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }

// ---------------------------------------------------------------------
// An [[assume]] whose operand cannot be evaluated without side effects is
// DECLINED, not rejected: the implementation simply does not get to assume
// anything, and the program is unaffected.  Previously this was
// "error: contract condition is not constant".
// ---------------------------------------------------------------------

constexpr bool modifying (int *p) { contract_assert ((++*p, true)); return true; }

constexpr int assume_operand_modifies ()
{
  int i = 0;
  [[assume (modifying (&i))]];	// operand is NOT evaluated
  return i;
}
static_assert (assume_operand_modifies () == 0);

// The same through a lambda, which reaches the evaluator differently: its
// closure is materialized during the evaluation, so unlike an
// already-constant call it always nests.
constexpr int assume_operand_modifies_lambda ()
{
  int i = 0;
  [[assume ([q = &i]{ contract_assert ((++*q, true)); return true; } ())]];
  return i;
}
static_assert (assume_operand_modifies_lambda () == 0);

// A FAILING contract inside an unevaluated operand must likewise not be
// reported: the operand is not evaluated, so there is no violation.
constexpr bool always_false (int *p) { contract_assert (*p > 100); return true; }

constexpr int assume_operand_violates ()
{
  int i = 0;
  [[assume (always_false (&i))]];
  return i;
}
static_assert (assume_operand_violates () == 0);

// ---------------------------------------------------------------------
// A contract predicate that modifies is evaluated twice -- once as the
// side-effect-free trial, once for real.  A violation recorded by a NESTED
// contract during the discarded trial must not survive it, or it is
// reported once per pass.
//
// obs() is observed, not enforced, so evaluation continues and the
// modification still stands: i ends at 1, and the violation is reported
// exactly once.
// ---------------------------------------------------------------------

constexpr bool obs (int *p) { contract_assert (*p > 100); return true; }  // { dg-warning "contract predicate is false" }

constexpr int nested_violation_across_rerun ()
{
  int i = 0;
  contract_assert (obs (&const_cast<int &> (i))			// { dg-warning "modifies" }
		   && (++const_cast<int &> (i), true));
  return i;
}
static_assert (nested_violation_across_rerun () == 1);
