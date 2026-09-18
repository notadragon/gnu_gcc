// Composition of [[assume]] and contract assertions during constant
// evaluation.  Both are evaluated under a modifiable_tracker, so they nest,
// and the two have DIFFERENT and deliberately asymmetric requirements:
//
//   * a contract predicate under a checking semantic IS evaluated, and its
//     side effects are real ([basic.contract.eval] carries an example whose
//     whole point is that they are observable);
//   * an [[assume]] operand is NOT evaluated ([dcl.attr.assume]), so its
//     side effects must never be observable.
//
// What must hold is that nesting COMPOSES: an inner construct never
// overrides the outer one's rule.  In particular a contract assertion
// reached while evaluating an [[assume]] operand is part of that operand's
// evaluation, so every modification it performs must still be discarded.
//
// Each case counts the modifications that survive, so a wrong answer is a
// wrong number rather than a diagnostic.  Deliveries are varied between a
// separate function and a lambda because those reach the evaluator
// differently -- a call whose arguments are already constants is evaluated
// in an earlier pass and never nests, while a lambda's closure is
// materialized during the evaluation and always does.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

constexpr bool M (int *p) { ++*p; return true; }

constexpr bool in_ctr (int *p) { contract_assert (M (p)); return true; }  // { dg-warning "modifies" }
constexpr bool in_asm (int *p) { [[assume (M (p))]]; return true; }
constexpr bool in_ctr0 (int *p) { contract_assert (p != nullptr); return true; }

// ---------------------------------------------------------------------
// Outer contract_assert: the predicate IS evaluated, so a modification
// before the nested construct, and one after it, are both observable.  A
// modification made INSIDE a nested contract_assert is observable too; one
// inside a nested [[assume]] is not.  Hence 3 and 2.
//
// Each of these warns exactly once per contract predicate that modifies the
// enclosing evaluation -- twice where a nested contract modifies as well.
// ---------------------------------------------------------------------

constexpr int ctr_in_ctr_fn ()
{
  int i = 0;
  contract_assert (M (&const_cast<int &> (i))			// { dg-warning "modifies" }
		   && in_ctr (&const_cast<int &> (i))
		   && M (&const_cast<int &> (i)));
  return i;
}
static_assert (ctr_in_ctr_fn () == 3);

constexpr int ctr_in_ctr_lambda ()
{
  int i = 0;
  contract_assert (M (&const_cast<int &> (i))			// { dg-warning "modifies" }
		   && [q = &const_cast<int &> (i)]
		      { contract_assert (M (q)); return true; } ()  // { dg-warning "modifies" }
		   && M (&const_cast<int &> (i)));
  return i;
}
static_assert (ctr_in_ctr_lambda () == 3);

constexpr int asm_in_ctr_fn ()
{
  int i = 0;
  contract_assert (M (&const_cast<int &> (i))			// { dg-warning "modifies" }
		   && in_asm (&const_cast<int &> (i))
		   && M (&const_cast<int &> (i)));
  return i;
}
static_assert (asm_in_ctr_fn () == 2);

constexpr int asm_in_ctr_lambda ()
{
  int i = 0;
  contract_assert (M (&const_cast<int &> (i))			// { dg-warning "modifies" }
		   && [q = &const_cast<int &> (i)]
		      { [[assume (M (q))]]; return true; } ()
		   && M (&const_cast<int &> (i)));
  return i;
}
static_assert (asm_in_ctr_lambda () == 2);

// ---------------------------------------------------------------------
// Outer [[assume]]: the operand is not evaluated, so NOTHING it does is
// observable -- whatever is nested inside it, and wherever the modification
// sits relative to that nesting.  These are the cases that regressed while
// ~modifiable_tracker cleared global->modifiable outright: the inner
// construct switched the assumption's suppression off and the modifications
// after it survived.
// ---------------------------------------------------------------------

constexpr int ctr_in_asm_fn ()
{
  int i = 0;
  [[assume (M (&const_cast<int &> (i))
	    && in_ctr (&const_cast<int &> (i))
	    && M (&const_cast<int &> (i)))]];
  return i;
}
static_assert (ctr_in_asm_fn () == 0);

constexpr int ctr_in_asm_lambda ()
{
  int i = 0;
  [[assume (M (&const_cast<int &> (i))
	    && [q = &const_cast<int &> (i)]
	       { contract_assert (M (q)); return true; } ()
	    && M (&const_cast<int &> (i)))]];
  return i;
}
static_assert (ctr_in_asm_lambda () == 0);

constexpr int asm_in_asm_fn ()
{
  int i = 0;
  [[assume (M (&const_cast<int &> (i))
	    && in_asm (&const_cast<int &> (i))
	    && M (&const_cast<int &> (i)))]];
  return i;
}
static_assert (asm_in_asm_fn () == 0);

constexpr int asm_in_asm_lambda ()
{
  int i = 0;
  [[assume (M (&const_cast<int &> (i))
	    && [q = &const_cast<int &> (i)]
	       { [[assume (M (q))]]; return true; } ()
	    && M (&const_cast<int &> (i)))]];
  return i;
}
static_assert (asm_in_asm_lambda () == 0);

// The decisive one: a nested contract that does NOT itself modify, followed
// by a modification.  Nothing short-circuits, so the enclosing assumption's
// suppression has to survive the inner assertion on its own.  This is the
// case a tracker destructor that clears rather than restores returns 1 for,
// silently.
constexpr int nonmod_ctr_then_modify_in_asm ()
{
  int i = 0;
  [[assume (in_ctr0 (&const_cast<int &> (i))
	    && M (&const_cast<int &> (i)))]];
  return i;
}
static_assert (nonmod_ctr_then_modify_in_asm () == 0);

// A constant, so a wrong answer changes object layout as well -- which is
// how this escapes into a program with no diagnostic at all.
char probe[nonmod_ctr_then_modify_in_asm () + 1];
static_assert (sizeof (probe) == 1);
