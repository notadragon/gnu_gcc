/* A contract predicate on a lambda that captures `this' read the closure
   object as if it were the enclosing class.

   remap_dummy_this_1 rewrote every tree for which is_this_parameter is true
   to DECL_ARGUMENTS of the function being emitted into.  In a lambda's
   operator() that first argument is __closure, so a member access in the
   predicate reinterpreted the closure:

     _1 = MEM[(struct S *)__closure].m;      // the predicate -- WRONG
     _2 = __closure->__this; _3 = _2->m;     // the body      -- right

   is_this_parameter is deliberately true for BOTH a real `this' PARM_DECL
   and a lambda's captured-`this' proxy -- a VAR_DECL named `this' whose
   DECL_VALUE_EXPR is already `__closure->__this'.  The proxy needs no
   remapping at all; the remap exists for the dummy `this' of a contract
   parsed on a declaration, and for re-pointing a real `this' at the first
   parameter of an outlined __pre_fn/__post_fn.

   Wrong code, not a crash, and the symptom is nondeterministic if you only
   watch whether the check fires: the predicate compares against a stack
   address.  So these tests report the value the predicate SAW.  */

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

struct S
{
  int m;

  /* The reported case: a lambda capturing `this', naming a member in its
     precondition.  */
  int pre_through_this ()
  {
    auto l = [this] () pre (probe (m)) { return m; };
    return l ();
  }

  /* The same in a postcondition.  */
  int post_through_this ()
  {
    auto l = [this] () post (r : probe (m)) { return m; };
    return l ();
  }

  /* Spelled with an explicit `this->'.  */
  int explicit_this ()
  {
    auto l = [this] () pre (probe (this->m)) { return m; };
    return l ();
  }

  /* A by-reference capture default also captures `this'.  */
  int ref_default ()
  {
    auto l = [&] () pre (probe (m)) { return m; };
    return l ();
  }

  /* A nested lambda: the inner one captures the outer closure, which
     captures `this'.  */
  int nested ()
  {
    auto outer = [this] () {
      auto inner = [this] () pre (probe (m)) { return m; };
      return inner ();
    };
    return outer ();
  }

  /* The control that the remap exists for: a contract on an ordinary member
     function, where `this' really is a PARM_DECL and must be re-pointed at
     the emitting function's first argument.  */
  int plain_member () pre (probe (m)) { return m; }
};

int
main ()
{
  S s { 42 };

  if (s.pre_through_this () != 42) __builtin_abort ();
  check_seen ("precondition through a captured this", 42);
  check_viol ("precondition through a captured this", 0);

  if (s.post_through_this () != 42) __builtin_abort ();
  check_seen ("postcondition through a captured this", 42);
  check_viol ("postcondition through a captured this", 0);

  if (s.explicit_this () != 42) __builtin_abort ();
  check_seen ("explicit this-> in a lambda predicate", 42);
  check_viol ("explicit this-> in a lambda predicate", 0);

  if (s.ref_default () != 42) __builtin_abort ();
  check_seen ("by-reference capture default", 42);
  check_viol ("by-reference capture default", 0);

  if (s.nested () != 42) __builtin_abort ();
  check_seen ("nested lambdas capturing this", 42);
  check_viol ("nested lambdas capturing this", 0);

  if (s.plain_member () != 42) __builtin_abort ();
  check_seen ("ordinary member function control", 42);
  check_viol ("ordinary member function control", 0);

  return failures;
}
