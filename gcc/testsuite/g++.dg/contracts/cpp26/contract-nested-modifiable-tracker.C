// Nested trackers: a contract assertion evaluated inside another tracked
// evaluation must not disturb the enclosing one.
//
// An [[assume]] operand is evaluated under a modifiable_tracker, and a
// contract predicate under a predicate_rollback; both nest.  A tracker
// destructor used to clear global->modifiable outright instead of restoring
// the enclosing tracker's set -- so once an inner assertion finished, the
// rest of the OUTER tracked evaluation ran untracked: its stores neither
// refused nor recorded, and so never rolled back.
//
// Which inner constructs actually reach the evaluator varies in ways that
// are not obvious from the source -- a call whose result comes from the
// constexpr call cache never re-enters its body, and an assertion whose
// predicate folds to a constant may never be evaluated at all.  The cases
// below deliberately cover several shapes rather than the one that happens
// to be understood, because the property being tested is that NONE of them
// disturbs the enclosing tracker.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct string
{
  const char *p;
  int i;
  constexpr string (const char *p): p (p), i (0) { }
  constexpr int length () { ++i; return __builtin_strlen (p); }
};

constexpr bool ctr_const (const char *) { contract_assert (true); return true; }
constexpr bool ctr_dyn (const char *p) { contract_assert (p != nullptr); return true; }

// ---------------------------------------------------------------------
// A contract predicate that modifies the enclosing evaluation.  The
// modification is permitted -- [basic.contract.eval] has this construct as a
// worked example -- so every case must be accepted, and the modification is
// undone once the predicate's value is known, so every case must leave
// i == 0, silently.  The const_cast is required: the predicate const-ifies
// the id-expression, not the object.
//
// The point is UNIFORMITY: the answer must not depend on the shape of the
// inner assertion.
// ---------------------------------------------------------------------

constexpr int plain ()
{
  string s ("foobar");
  contract_assert (const_cast<string &> (s).length () > 0);
  return s.i;
}
static_assert (plain () == 0);

constexpr int via_call_const ()
{
  string s ("foobar");
  contract_assert (ctr_const (s.p)
		   && const_cast<string &> (s).length () > 0);
  return s.i;
}
static_assert (via_call_const () == 0);

constexpr int via_call_dyn ()
{
  string s ("foobar");
  contract_assert (ctr_dyn (s.p)
		   && const_cast<string &> (s).length () > 0);
  return s.i;
}
static_assert (via_call_dyn () == 0);

constexpr int via_lambda ()
{
  string s ("foobar");
  contract_assert ([]{ contract_assert (true); return true; } ()
		   && const_cast<string &> (s).length () > 0);
  return s.i;
}
static_assert (via_lambda () == 0);

constexpr int via_lambda_capture ()
{
  string s ("foobar");
  contract_assert ([&]{ contract_assert (true); return true; } ()
		   && const_cast<string &> (s).length () > 0);
  return s.i;
}
static_assert (via_lambda_capture () == 0);

// ---------------------------------------------------------------------
// The mirror image, and the case that produces WRONG CODE rather than a
// rejection.  An [[assume]] operand is not evaluated, so x must stay 0.  A
// contract assertion reached while evaluating that operand must not switch
// the assumption's suppression off, or bump()'s increment survives and these
// come out 1, silently and at any warning level.
// ---------------------------------------------------------------------

constexpr bool bump (unsigned *p) { *p += 1; return true; }
constexpr bool a_ctr_const (unsigned *) { contract_assert (true); return true; }
constexpr bool a_ctr_dyn (unsigned *p) { contract_assert (p != nullptr); return true; }
constexpr bool a_assume (unsigned *p) { [[assume (*p < 100)]]; return true; }

constexpr unsigned assume_plain ()
{
  unsigned x = 0;
  [[assume (bump (&x))]];
  return x;
}
static_assert (assume_plain () == 0);

constexpr unsigned assume_in_assume ()
{
  unsigned x = 0;
  [[assume (a_assume (&x) && bump (&x))]];
  return x;
}
static_assert (assume_in_assume () == 0);

constexpr unsigned contract_in_assume_const ()
{
  unsigned x = 0;
  [[assume (a_ctr_const (&x) && bump (&x))]];
  return x;
}
static_assert (contract_in_assume_const () == 0);

constexpr unsigned contract_in_assume_dyn ()
{
  unsigned x = 0;
  [[assume (a_ctr_dyn (&x) && bump (&x))]];
  return x;
}
static_assert (contract_in_assume_dyn () == 0);

constexpr unsigned contract_in_assume_lambda ()
{
  unsigned x = 0;
  [[assume ([&]{ contract_assert (true); return true; } () && bump (&x))]];
  return x;
}
static_assert (contract_in_assume_lambda () == 0);

// The value is a constant, so getting it wrong changes object layout too --
// which is how the defect escapes into a program without any diagnostic.
char probe[contract_in_assume_const () + 1];
static_assert (sizeof (probe) == 1);
