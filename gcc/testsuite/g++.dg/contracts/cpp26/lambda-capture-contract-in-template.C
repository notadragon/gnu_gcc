/* A contract on a lambda that CAPTURES, inside a template, crashed.

   Two things were wrong, and the first hid the second.

   1. The predicate names the PATTERN lambda's capture proxy, and nothing maps
      it to the instantiation's.  insert_capture_proxy registers the
      instantiation's proxy under the variable it captures -- which is by then
      the substituted one -- so the pattern's proxy is never itself a key, and
      the lookup finds nothing.  Substitution then fell through to its
      "parameter used in a late-specified return type" recovery, whose
      gcc_assert (cp_unevaluated_operand) does not hold for a predicate.
      Getting from the pattern's proxy to the instantiation's takes two hops
      through local_specializations: the pattern's proxy stands for a variable
      of the enclosing function, that variable's specialization is the
      instantiated variable, and the instantiated proxy is registered under
      *that*.  Stopping after one hop lands on the instantiated variable, an
      automatic of the containing function, which is then rightly refused.

   2. Once the reference resolves, process_outer_var_ref rejected it anyway.
      It carried a carve-out for a contract condition naming a PARAMETER and
      another for a postcondition naming a variable, but none for a
      precondition naming a capture.

   Before the fix this crashed with a segfault on stock 16.2.0 and on trunk,
   so nothing here is branch-specific.

   Every check reports the value the PREDICATE saw.  That matters most for
   `copy_not_alias' below: a by-value capture is a copy taken when the closure
   is built, so a predicate that read the enclosing variable instead would
   still compile, still fire no violation, and still be wrong.  */

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

template <class T>
int
by_value (T a)
{
  int lo = 5;
  auto l = [lo] (int b) pre (probe (b - lo)) { return b; };
  return l ((int) a);
}

template <class T>
int
by_ref (T a)
{
  int lo = 100;
  auto l = [&lo] (int b) pre (probe (b + lo)) { return b; };
  return l ((int) a);
}

template <class T>
int
two_captures (T a)
{
  int p = 3, q = 40;
  auto l = [p, q] (int b) pre (probe (b + p + q)) { return b; };
  return l ((int) a);
}

/* A capture-default.  The body odr-uses `lo' as well, deliberately: a lambda
   may not acquire a capture *solely* because a contract assertion names it,
   since that would change the closure type, and the predicate-only spelling
   is correctly rejected (lambda-capture-in-contract-error.C covers that).  */
template <class T>
int
capture_default (T a)
{
  int lo = 7;
  auto l = [=] (int b) pre (probe (b * lo)) { return b + lo; };
  return l ((int) a);
}

/* A postcondition naming both a capture and the result binding.  */
template <class T>
int
post_capture (T a)
{
  int lo = 2;
  auto l = [lo] (int b) post (r : probe (r + lo)) { return b * 10; };
  return l ((int) a);
}

/* The predicate must see the captured COPY.  Mutating the variable after the
   closure is built has to be invisible to it.  */
template <class T>
int
copy_not_alias (T a)
{
  int lo = 1;
  auto l = [lo] (int b) pre (probe (lo)) { return b; };
  lo = 999;
  return l ((int) a);
}

template <class T>
struct S
{
  int m (T a)
  {
    int lo = 6;
    auto l = [lo] (int b) pre (probe (b - lo)) { return b; };
    return l ((int) a);
  }
};

/* Control: the same lambda outside a template.  This already worked; it is
   here so a fix cannot regress it.  */
static int
non_template ()
{
  int lo = 4;
  auto l = [lo] (int b) pre (probe (b - lo)) { return b; };
  return l (10);
}

/* The predicate must still be able to fail.  */
template <class T>
int
violates (T a)
{
  int hi = 100;
  auto l = [hi] (int b) pre (b > hi) { return b; };
  return l ((int) a);
}

int
main ()
{
  if (by_value (15) != 15) __builtin_abort ();
  check_seen ("by-value capture in a function template", 10);
  if (by_value (25.0) != 25) __builtin_abort ();
  check_seen ("by-value capture, second instantiation", 20);

  if (by_ref (1) != 1) __builtin_abort ();
  check_seen ("by-reference capture", 101);

  if (two_captures (1) != 1) __builtin_abort ();
  check_seen ("two captures", 44);

  if (capture_default (3) != 10) __builtin_abort ();
  check_seen ("capture default", 21);

  if (post_capture (5) != 50) __builtin_abort ();
  check_seen ("postcondition naming a capture and the result", 52);

  if (copy_not_alias (0) != 0) __builtin_abort ();
  check_seen ("the capture is a copy, not an alias", 1);

  { S<int> s; if (s.m (16) != 16) __builtin_abort (); }
  check_seen ("capture in a class-template member", 10);
  { S<long> s; if (s.m (26) != 26) __builtin_abort (); }
  check_seen ("class-template member, second instantiation", 20);

  if (non_template () != 10) __builtin_abort ();
  check_seen ("control: the same lambda outside a template", 6);

  if (viol != 0)
    {
      std::printf ("FAIL: unexpected violations: %d\n", viol);
      ++failures;
    }
  viol = 0;

  if (violates (1) != 1) __builtin_abort ();
  if (viol != 1)
    {
      std::printf ("FAIL: a failing predicate did not report\n");
      ++failures;
    }

  if (failures)
    __builtin_abort ();
  return 0;
}
