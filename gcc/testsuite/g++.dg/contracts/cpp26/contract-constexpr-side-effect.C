/* A contract predicate that modifies an object of the enclosing constant
   evaluation is still a core constant expression, and must not be reported
   as non-constant; and the modification does not reach the enclosing
   evaluation.

   [basic.contract.eval] says an evaluation using a checking semantic
   "determines the value of the predicate", that "it is unspecified whether
   the predicate is evaluated", and that an alternative evaluation "that
   produces the same value as the predicate but has no side effects can
   occur".  A contract violation on the grounds of constant evaluation occurs
   only when "the predicate is not a core constant expression".

   Modifying an object whose lifetime began within the enclosing constant
   evaluation is perfectly good in a core constant expression, so a
   side-effecting predicate is well-formed.  GCC evaluates the predicate to
   determine its value and then undoes whatever it modified, so the
   enclosing evaluation proceeds as if the predicate had not been evaluated
   (the side-effect-free evaluation the wording permits), and says nothing
   about it.  At run time the predicate is evaluated, side effect and all.

   Regression test: GCC used to evaluate the predicate under a
   modifiable_tracker, which refuses such a modification, and then read that
   refusal as "not a core constant expression" ("contract condition is not
   constant"), rejecting a valid program.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr bool
bump (unsigned *p) { *p += 1; return true; }

/* The predicate modifies an object belonging to the caller's evaluation.  */
constexpr unsigned
one_side_effect (unsigned *p)
{
  contract_assert (bump (p));
  return *p;
}

constexpr unsigned
run_one ()
{
  unsigned x = 1;
  return one_side_effect (&x);
}

static_assert (run_one () == 1);

/* A precondition doing the same, and reached twice, so the rollback is not
   a one-shot.  */
constexpr unsigned
pre_side_effect (unsigned *p) pre (bump (p))
{
  return *p;
}

constexpr unsigned
run_pre ()
{
  unsigned x = 0;
  unsigned a = pre_side_effect (&x);
  unsigned b = pre_side_effect (&x);
  return a * 10 + b;
}

static_assert (run_pre () == 0);

/* The modification and a genuine read of enclosing state together: the
   predicate sees its own modification (each evaluation's hits <= limit
   reads 1), and the caller never does.  */
struct counter { unsigned hits; };

constexpr bool
touch (counter *c, unsigned limit) { c->hits += 1; return c->hits <= limit; }

constexpr unsigned
guarded (counter *c)
{
  contract_assert (touch (c, 10));
  return c->hits;
}

constexpr unsigned
run_guarded ()
{
  counter c { 0 };
  guarded (&c);
  guarded (&c);
  return guarded (&c);
}

static_assert (run_guarded () == 0);

/* Control: a predicate that is genuinely not a core constant expression is
   still a violation.  Kept here as a run-time-only path so this file stays
   a run test; basic.contract.eval.p7.3.C covers the diagnostic itself.  */
extern bool opaque ();

unsigned
not_constant (unsigned *p)
{
  contract_assert (opaque () || bump (p));
  return *p;
}

bool
opaque () { return true; }

int
main ()
{
  /* The same functions at run time: the predicate runs, side effect and
     all, exactly once per evaluation.  */
  unsigned x = 1;
  if (one_side_effect (&x) != 2)
    __builtin_abort ();
  if (x != 2)
    __builtin_abort ();

  unsigned y = 0;
  if (pre_side_effect (&y) != 1)
    __builtin_abort ();
  if (pre_side_effect (&y) != 2)
    __builtin_abort ();

  counter c { 0 };
  if (guarded (&c) != 1)
    __builtin_abort ();

  unsigned z = 5;
  if (not_constant (&z) != 5)
    __builtin_abort ();

  return 0;
}
