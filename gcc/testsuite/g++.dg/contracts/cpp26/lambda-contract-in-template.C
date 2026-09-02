/* A lambda's contract specifier was never substituted when the lambda was
   instantiated as part of a template.

   tsubst_function_decl copies a function's contract specifiers onto the
   instantiation WITHOUT substituting them -- "NOTE these are not substituted
   at this point" -- because for an ordinary function
   regenerate_decl_from_template substitutes them later.  A lambda's
   operator() never reaches regenerate_decl_from_template: tsubst_lambda_expr
   builds it and substitutes its body on the spot.  So the instantiation kept
   the PATTERN's contract trees, whose predicate names the pattern's
   PARM_DECLs and whose result binding belongs to the pattern.

   One defect, four faces, depending on where the stale tree was first used:

     pre, non-generic lambda            expand_expr_real_1 -- no RTL for `b'
     pre, generic lambda                tsubst_expr's cp_unevaluated_operand
					  assert -- no local specialization
     post with a result name, once      rebuild_postconditions' assert on
					  DECL_CONTEXT (checking builds only)
     post with a result name, twice     tsubst_expr -- the first instantiation
					  had silently reparented the
					  pattern's result variable onto
					  itself, and the second tripped over
					  it (RELEASE builds too)

   Nesting and generic lambdas are NOT part of the trigger, though the bug was
   first written up as being about them: the report had been classified on a
   release build, where the postcondition face is compiled out and looked like
   a working control.  The three ingredients are that the lambda carries a
   contract SPECIFIER (a contract_assert in the body substitutes with the
   body), that the predicate names the lambda's own parameter or result, and
   that the lambda is instantiated as part of a template.

   Compiling clean is not the property under test -- every check below reports
   the value the PREDICATE saw.  Three of the four faces were ICEs, but face 4
   proves the postcondition half also produced a correct-looking program that
   only broke on the second instantiation.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int viol = 0;
static int failures = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++viol; }

static int seen = -1;

/* A predicate cannot assign to a variable -- the contract constifies what it
   names -- so record through a call.  */
static bool
probe (int observed)
{
  seen = observed;
  return true;
}

static void
check_seen (const char *what, int want)
{
  if (seen != want)
    {
      std::printf ("FAIL: %s: predicate saw %d, expected %d\n",
		   what, seen, want);
      ++failures;
    }
  seen = -1;
}

static void
check_viol (const char *what, int want)
{
  if (viol != want)
    {
      std::printf ("FAIL: %s: expected %d violations, got %d\n",
		   what, want, viol);
      ++failures;
    }
  viol = 0;
}

/* Face 1 -- a precondition naming a parameter, on a plain lambda in a
   function template.  The smallest shape: no nesting, nothing generic.  */
template <class T>
int
pre_plain (T a)
{
  auto l = [] (int b) pre (probe (b)) { return b; };
  return l ((int) a);
}

/* Face 2 -- the same on a generic lambda, whose operator() is substituted
   later, from its own instantiation.  */
template <class T>
int
pre_generic (T a)
{
  auto l = [] (auto b) pre (probe (b)) { return b; };
  return l ((int) a);
}

/* Faces 3 and 4 -- a postcondition with a result name.  Instantiated more
   than once below, which is what reached a release build.  */
template <class T>
int
post_result (T a)
{
  auto l = [] (int b) post (r : probe (r)) { return b * 10; };
  return l ((int) a);
}

/* A lambda in a class-template member -- the same defect, reached without a
   function template.  */
template <class T>
struct S
{
  int m (T a)
  {
    auto l = [] (int b) pre (probe (b)) { return b; };
    return l ((int) a);
  }
};

/* A lambda nested inside a generic lambda: the shape the bug was originally
   reported as, kept so it stays covered.  */
static int
nested_in_generic (int a)
{
  auto outer = [] (auto x) {
    auto inner = [] (int b) pre (probe (b)) { return b; };
    return inner ((int) x);
  };
  return outer (a);
}

/* NOT COVERED HERE: a predicate naming a CAPTURE rather than a parameter,
   in a template --

     template <class T> int f (T a)
     { int lo = 5; auto l = [lo] (int b) pre (b > lo) { return b; };
       return l ((int) a); }

   -- which is a separate, pre-existing defect: the pattern's capture proxy
   has no local specialization either, and substitution reaches the same
   recovery path.  It segfaults on stock 16.2.0, on trunk and on this branch
   before the fix above, so it is not a regression from it.  The non-template
   spelling works and is covered by lambda-capture-in-contract.C.  */

/* The predicate must still be able to FAIL: a fix that stopped evaluating it
   would pass every check above.  */
template <class T>
int
pre_violates (T a)
{
  auto l = [] (int b) pre (b > 100) { return b; };
  return l ((int) a);
}

/* Controls.  Each drops exactly one necessary ingredient and was always
   clean; they are here so a fix cannot regress them.  */

static int g_v = 7;

template <class T>
int
ctl_names_nothing_local (T a)
{
  auto l = [] (int b) pre (probe (99)) { return b; };
  return l ((int) a);
}

template <class T>
int
ctl_names_a_global (T a)
{
  auto l = [] (int b) pre (probe (g_v)) { return b; };
  return l ((int) a);
}

/* A contract_assert is part of the body and substitutes with it.  */
template <class T>
int
ctl_assert_in_body (T a)
{
  auto l = [] (int b) { contract_assert (probe (b)); return b; };
  return l ((int) a);
}

static int
ctl_non_template ()
{
  auto l = [] (int b) pre (probe (b)) { return b; };
  return l (33);
}

/* A contract on the template's own function goes through
   regenerate_decl_from_template and was never affected.  */
template <class T>
int
ctl_on_the_template (T a) pre (probe ((int) a))
{
  return (int) a;
}

int
main ()
{
  if (pre_plain (7) != 7) __builtin_abort ();
  check_seen ("pre, plain lambda in a function template", 7);
  check_viol ("pre, plain lambda in a function template", 0);

  /* A second call to the same instantiation, then a second instantiation:
     face 4 was invisible until the template was instantiated twice.  */
  if (pre_plain (9) != 9) __builtin_abort ();
  check_seen ("pre, same instantiation called again", 9);
  if (pre_plain (4.0) != 4) __builtin_abort ();
  check_seen ("pre, second instantiation (T = double)", 4);

  if (pre_generic (8) != 8) __builtin_abort ();
  check_seen ("pre, generic lambda in a function template", 8);
  if (pre_generic (2.0) != 2) __builtin_abort ();
  check_seen ("pre, generic lambda, second instantiation", 2);

  if (post_result (7) != 70) __builtin_abort ();
  check_seen ("post result binding, function template", 70);
  check_viol ("post result binding, function template", 0);
  if (post_result (3.0) != 30) __builtin_abort ();
  check_seen ("post result binding, second instantiation", 30);
  if (post_result (2L) != 20) __builtin_abort ();
  check_seen ("post result binding, third instantiation", 20);

  { S<int> s; if (s.m (11) != 11) __builtin_abort (); }
  check_seen ("pre, lambda in a class-template member", 11);
  { S<long> s; if (s.m (12) != 12) __builtin_abort (); }
  check_seen ("pre, class template, second instantiation", 12);

  if (nested_in_generic (6) != 6) __builtin_abort ();
  check_seen ("pre, lambda nested in a generic lambda", 6);

  if (pre_violates (1) != 1) __builtin_abort ();
  check_viol ("a predicate that must fail", 1);

  if (ctl_names_nothing_local (1) != 1) __builtin_abort ();
  check_seen ("control: predicate names nothing local", 99);
  if (ctl_names_a_global (1) != 1) __builtin_abort ();
  check_seen ("control: predicate names a global", 7);
  if (ctl_assert_in_body (5) != 5) __builtin_abort ();
  check_seen ("control: contract_assert in the body", 5);
  if (ctl_non_template () != 33) __builtin_abort ();
  check_seen ("control: the same lambda in a non-template", 33);
  if (ctl_on_the_template (44) != 44) __builtin_abort ();
  check_seen ("control: contract on the template itself", 44);
  check_viol ("controls", 0);

  if (failures)
    __builtin_abort ();
  return 0;
}
