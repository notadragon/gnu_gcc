/* A mutation performed through a postcondition's result binding is observable
   by the caller.

   The position this encodes (2026-09-02): a predicate we evaluate is evaluated
   faithfully, so its effects stand.  [basic.contract.eval] permits *not
   evaluating* a predicate -- an alternative evaluation producing the same value
   with no side effects may be substituted -- but that is all-or-nothing.  It
   does not license evaluating a predicate and then discarding what it did.

   GCC gets every shape below right, and did before this test existed.  It is a
   MIRROR of clang/test/Contracts/Runnable/contract-result-binding-mutation.cpp
   and of its companion ...-mutation-class.cpp, added when Clang was found to
   discard these writes: its epilogue materialised the return value before
   running the postconditions and returned the stale copy.  Clang now observes
   the scalar shapes; the CLASS-typed ones are still lost there and are pinned
   XFAIL on that side.  Nothing pinned the GCC side at all -- contract-result-
   binding-identity.C covers *which object* the binding names, not whether a
   write through it survives -- so a regression here would have been silent.

   Every check reports what the CALLER sees, which is the whole property.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &) { }

static int failures = 0;

static void
check (const char *what, int got, int want)
{
  if (got != want)
    {
      std::printf ("FAIL: %s: caller saw %d, expected %d\n", what, got, want);
      ++failures;
    }
}

/* Increment through a const_cast.  */
int direct_increment () post (r : (const_cast<int &> (r)++, true)) { return 1; }

/* Assignment rather than increment.  */
int assignment () post (r : (const_cast<int &> (r) = 42, true)) { return 0; }

/* The mutation performed inside a called function taking `const &'.  This
   spelling reaches the binding through a reference parameter, which is where
   the binding used to denote a second object (PR112794).  */
static bool
bump (const int &r)
{
  const_cast<int &> (r) += 10;
  return true;
}
int via_reference_parameter () post (r : bump (r)) { return 5; }

/* Two postconditions, each mutating: both effects stand, in order.  */
int two_postconditions () post (r : (const_cast<int &> (r)++, true))
			 post (r : (const_cast<int &> (r) *= 3, true))
{ return 2; }

/* A postcondition with no result name is unaffected.  */
int no_result_name () post (true) { return 77; }

/* Class-typed results: one returned indirectly, one small enough to come back
   in registers.  These are the two Clang still loses.  */
struct Big { int a, b, c, d; };
struct Small { int a; };

static bool
bump_big (const Big &r)
{
  const_cast<Big &> (r).a += 7;
  return true;
}
static bool
bump_small (const Small &r)
{
  const_cast<Small &> (r).a += 9;
  return true;
}

Big big () post (r : bump_big (r)) { return Big { 1, 2, 3, 4 }; }
Small small_ () post (r : bump_small (r)) { return Small { 1 }; }

/* A class-typed result with a NON-TRIVIAL copy constructor and destructor
   must be bound WITHOUT introducing a temporary -- [dcl.contract.res]
   Example 2's `B' row, "the postcondition check succeeds, no temporary is
   introduced".  An extra copy here would be observable as extra constructor
   and destructor calls, so pin the counts and not merely the address.

   Example 2's `A' row permits a temporary otherwise, and both compilers do
   introduce one for a TRIVIALLY-copyable class (measured 2026-09-03: the
   predicate sees a different address on both).  That costs no constructor
   call; the only consequence is whether a mutation through it survives, which
   the Big/Small cases above cover.  */
struct Counted
{
  int a;
  static int ctor, copy, move;
  Counted (int v) : a (v) { ++ctor; }
  Counted (const Counted &o) : a (o.a) { ++copy; }
  Counted (Counted &&o) : a (o.a) { ++move; }
};
int Counted::ctor = 0, Counted::copy = 0, Counted::move = 0;

static const Counted *seen_addr = 0;
static bool
note_counted (const Counted &r)
{
  seen_addr = &r;
  return true;
}
Counted counted () post (r : note_counted (r)) { return Counted (1); }

/* A function returning a REFERENCE, whose result name is mutated.  Clang
   crashes in codegen on merely naming the result of a reference-returning
   function; GCC handles it.  */
static int g_ref = 100;
int &reference_return () post (r : (const_cast<int &> (r) += 5, true))
{ return g_ref; }

int
main ()
{
  check ("increment through const_cast", direct_increment (), 2);
  check ("assignment through const_cast", assignment (), 42);
  check ("mutation via a const& parameter", via_reference_parameter (), 15);
  check ("two mutating postconditions", two_postconditions (), 9);
  check ("no result name", no_result_name (), 77);
  check ("class returned indirectly", big ().a, 8);
  check ("small class returned in registers", small_ ().a, 10);
  check ("reference return", reference_return (), 105);

  {
    Counted::ctor = Counted::copy = Counted::move = 0;
    Counted c = counted ();
    check ("non-trivial result: constructions", Counted::ctor, 1);
    check ("non-trivial result: copies", Counted::copy, 0);
    check ("non-trivial result: moves", Counted::move, 0);
    if (seen_addr != &c)
      {
	std::printf ("FAIL: non-trivial result: the predicate saw a temporary,"
		     " not the returned object\n");
	++failures;
      }
  }

  if (failures)
    __builtin_abort ();
  return 0;
}
