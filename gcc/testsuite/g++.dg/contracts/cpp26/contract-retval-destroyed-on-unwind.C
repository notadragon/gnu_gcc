/* Once the returned object has been initialized, unwinding must destroy it --
   exactly once, whichever way the function is left.

   Two ways out after the returned object exists:

     (a) a local variable's destructor throws.  [except.ctor]/2 requires the
         returned object to be destroyed: "If an exception is thrown during
         the destruction of temporaries or local variables for a return
         statement, the destructor for the returned object (if any) is also
         invoked."  GCC has always done this, via current_retval_sentinel.

     (b) a contract-violation handler throws out of a POSTcondition check.
         [stmt.return]/5 sequences postcondition evaluation after the
         destruction of local variables, so the returned object exists here
         too -- but no wording requires its destruction: [except.ctor]/2 names
         only temporaries and local variables and says nothing about contract
         assertions, and [basic.contract.eval] says the behaviour is "as if
         the function body exits via that same exception", describing a state
         in which the result object was never initialized.  We destroy it
         anyway.  Leaking an object the program can no longer reach is not a
         defensible reading, and a core issue is owed; do NOT "fix" this test
         back to expecting a leak on the strength of the wording.

   Every case counts constructions against destructions, so a leak and a
   double destroy both fail, and the two cleanups involved -- the body's
   sentinel-guarded one and the postcondition one -- are checked not to
   overlap.  A count-free compile test would catch neither.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int live = 0;
int destroyed = 0;

struct Counted {
  int v;
  Counted (int x) : v (x) { ++live; }
  Counted (const Counted &o) : v (o.v) { ++live; }
  ~Counted () { --live; ++destroyed; }
};

struct Trivial { int v; };

struct E { };

/* A handler that throws, so a violation leaves the function by exception.  */
bool handler_should_throw = false;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  if (handler_should_throw)
    throw E { };
}

struct ThrowOnDestroy {
  bool armed;
  ~ThrowOnDestroy () noexcept (false) { if (armed) throw E { }; }
};

/* --- (a) a local's destructor throws, with and without NRVO ------------- */

Counted
local_throws_nrvo (bool arm)
{
  ThrowOnDestroy guard { arm };
  Counted result (1);
  return result;			/* NRVO candidate */
}

Counted
local_throws_no_nrvo (bool arm)
{
  ThrowOnDestroy guard { arm };
  Counted a (1), b (2);
  return arm ? a : b;			/* not an NRVO candidate */
}

Counted
local_throws_two_locals (bool arm)
{
  ThrowOnDestroy outer { arm };
  Counted keepalive (9);
  ThrowOnDestroy inner { false };
  Counted result (1);
  return result;
}

/* --- (b) a postcondition handler throws, with and without NRVO ---------- */

Counted
post_throws_nrvo (int n) post (r : r.v > 100)
{
  Counted result (n);
  return result;
}

Counted
post_throws_no_nrvo (int n) post (r : r.v > 100)
{
  Counted a (n), b (n + 1);
  return n > 0 ? a : b;
}

/* A precondition failing must NOT destroy anything: the returned object does
   not exist yet.  */
Counted
pre_throws (int n) pre (n > 100)
{
  Counted result (n);
  return result;
}

/* --- controls ----------------------------------------------------------- */

Counted
post_passes (const int n) post (r : r.v == n)
{
  Counted result (n);
  return result;
}

Trivial
trivial_return (int n) post (r : r.v > 100)
{
  Trivial t { n };
  return t;
}

void
void_return (const int n) post (n > 100)
{
}

Counted
no_contract (bool arm)
{
  ThrowOnDestroy guard { arm };
  Counted result (1);
  return result;
}

static void
check (int got, int want)
{
  if (got != want)
    __builtin_abort ();
}

/* Run F, expecting it to leave by an exception, and require that it balanced
   its constructions against its destructions.  */
template <class F>
static void
expect_throw_balanced (F f)
{
  live = 0;
  destroyed = 0;
  bool threw = false;
  try { f (); } catch (E &) { threw = true; }
  if (!threw)
    __builtin_abort ();
  check (live, 0);			/* 0 means: no leak, no double destroy */
  if (destroyed == 0)
    __builtin_abort ();			/* something must have been destroyed */
}

int
main ()
{
  /* (a) [except.ctor]/2 -- required by the standard.  */
  handler_should_throw = false;
  expect_throw_balanced ([] { local_throws_nrvo (true); });
  expect_throw_balanced ([] { local_throws_no_nrvo (true); });
  expect_throw_balanced ([] { local_throws_two_locals (true); });
  expect_throw_balanced ([] { no_contract (true); });

  /* (b) a throwing violation handler on a postcondition.  */
  handler_should_throw = true;
  expect_throw_balanced ([] { post_throws_nrvo (1); });
  expect_throw_balanced ([] { post_throws_no_nrvo (1); });

  /* A failing PREcondition: the returned object does not exist yet, so
     nothing may be destroyed -- and nothing may be leaked either.  */
  live = 0;
  destroyed = 0;
  bool threw = false;
  try { pre_throws (1); } catch (E &) { threw = true; }
  if (!threw)
    __builtin_abort ();
  check (live, 0);
  check (destroyed, 0);

  /* Controls: normal completion destroys the object exactly once, when the
     caller is done with it -- not early, and not twice.  */
  handler_should_throw = false;
  live = 0;
  destroyed = 0;
  {
    Counted got = post_passes (7);
    check (got.v, 7);
    check (live, 1);			/* still alive: the caller holds it */
    check (destroyed, 0);
  }
  check (live, 0);
  check (destroyed, 1);

  /* A returned object with a trivial destructor, and a void return, must not
     grow a cleanup.  Both have a failing postcondition and a throwing
     handler, so they exercise the same path.  */
  handler_should_throw = true;
  threw = false;
  try { trivial_return (1); } catch (E &) { threw = true; }
  if (!threw)
    __builtin_abort ();
  threw = false;
  try { void_return (1); } catch (E &) { threw = true; }
  if (!threw)
    __builtin_abort ();

  /* Normal return with no violation at all, once more, to pin that the new
     cleanup is EH-only.  */
  handler_should_throw = false;
  live = 0;
  destroyed = 0;
  check (no_contract (false).v, 1);
  check (live, 0);
  check (destroyed, 1);

  return 0;
}
