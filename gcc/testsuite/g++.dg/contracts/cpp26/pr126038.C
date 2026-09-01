/* PR c++/126038 -- a PRECONDITION on a lambda that named one of the
   lambda's captures ICEd:

     internal compiler error: in gimplify_var_or_parm_decl, at gimplify.cc:3298

   That assertion catches a VAR_DECL the gimplifier has not seen declared in
   any BIND_EXPR.  The decl was the capture proxy, whose DECL_VALUE_EXPR is
   __closure->__a -- it would have resolved six lines further on, had it been
   in scope.

   maybe_apply_function_contracts already had code to keep the proxies in
   scope for the contracts, but it only recognised a body that was itself a
   BIND_EXPR; the proxy-declaring BIND_EXPR also arrives wrapped in a
   single-statement STATEMENT_LIST.  In that case the hoist was skipped and
   the precondition was emitted as a PRECEDING SIBLING of the bind that
   declares the proxies.  Postconditions were unaffected -- they are emitted
   after the body, by which point the gimplifier has walked that bind, which
   is why only `pre' ever crashed.

   All three explicit capture forms reached it: by reference, by copy, and
   an init-capture.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int viol = 0;
static int failures = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++viol; }

static void
check (const char *what, int want)
{
  if (viol != want)
    {
      std::printf ("FAIL: %s: expected %d violations, got %d\n",
		   what, want, viol);
      ++failures;
    }
  viol = 0;
}

static void
check_eq (const char *what, int got, int want)
{
  if (got != want)
    {
      std::printf ("FAIL: %s: expected %d, got %d\n", what, want, got);
      ++failures;
    }
}

/* A capture of `this' is deliberately NOT covered here.  It compiles, but
   the predicate reads the wrong object: remap_dummy_this rewrites every
   is_this_parameter tree to DECL_ARGUMENTS of the function being emitted
   into, which in a lambda's operator() is __closure, so a member access in
   the predicate reinterprets the closure as the enclosing class.  That is a
   separate wrong-code bug, older than this one -- see the GCC-10 entry in
   the p3850impl pending reports -- and pinning it here would either encode
   the wrong answer or fail for an unrelated reason.  */

int
main ()
{
  int a = 1, b = 2;

  /* The reporter's case: a by-reference capture named in the
     precondition, with a by-copy capture used in the body.  */
  auto by_ref = [&a, b] (int v) pre (v > a) { return v + b; };
  check_eq ("by-reference capture in a precondition", by_ref (5), 7);
  check ("by-reference capture, precondition satisfied", 0);
  by_ref (0);
  check ("by-reference capture, precondition violated", 1);

  /* A by-copy capture named in the precondition.  */
  auto by_copy = [a] (int v) pre (v > a) { return v; };
  check_eq ("by-copy capture in a precondition", by_copy (5), 5);
  check ("by-copy capture, precondition satisfied", 0);
  by_copy (0);
  check ("by-copy capture, precondition violated", 1);

  /* An init-capture named in the precondition.  */
  auto by_init = [c = 1] (int v) pre (v > c) { return v + c; };
  check_eq ("init-capture in a precondition", by_init (5), 6);
  check ("init-capture, precondition satisfied", 0);
  by_init (0);
  check ("init-capture, precondition violated", 1);

  /* A by-reference capture must see later changes to the captured
     object, in the predicate as much as in the body.  */
  a = 10;
  by_ref (5);
  check ("by-reference capture tracks the referent", 1);
  a = 1;

  /* Both a precondition and a postcondition on the same lambda.  */
  auto both = [a] (int v) pre (v > a) post (r : r > a) { return v + 1; };
  check_eq ("pre and post on one lambda", both (5), 6);
  check ("pre and post on one lambda, both satisfied", 0);

  /* Postconditions were never broken; keep a control for them.  */
  auto post_only = [&a] (int v) post (r : r > a) { return v; };
  check_eq ("capture in a postcondition control", post_only (5), 5);
  check ("capture in a postcondition control, satisfied", 0);
  post_only (0);
  check ("capture in a postcondition control, violated", 1);

  /* A lambda with contracts but no captures at all -- the shape that
     already worked, kept as the control for the hoist itself.  */
  auto no_cap = [] (int v) pre (v > 0) { return v; };
  check_eq ("no captures control", no_cap (5), 5);
  check ("no captures control, satisfied", 0);

  return failures;
}
