// Postcondition captures work with -fno-exceptions.
//
// They used to fail to COMPILE, for every capture shape:
//
//   error: exception handling disabled, use '-fexceptions' to enable
//
// pointed at the closing brace.  contracts.cc lowers capture initialization
// inside a try/catch, so that a throwing capture initializer becomes a
// post_capture violation (P3098's phase model), and both capture sites -- the
// ordinary one and the virtual capture struct -- called begin_handler
// unconditionally; doing_eh() in cp/except.cc then errors out when
// flag_exceptions is off.  With no exceptions nothing can throw, so the
// try/catch is no longer built at all.
//
// This is a run test, not a compile test.  What matters is that the captures
// are initialized, observed by the predicate, and destroyed -- a test that
// only compiled would pass against a fix that emitted no capture at all.
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fno-exceptions -fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

static int observed = 0;

static bool
note (int v)
{
  observed = v;
  return true;
}

struct TrivialDtor
{
  int v;
};

struct NonTrivialDtor
{
  static int live;
  int v;
  NonTrivialDtor (int v) : v (v) { ++live; }
  NonTrivialDtor (const NonTrivialDtor &o) : v (o.v) { ++live; }
  ~NonTrivialDtor () { --live; }
};
int NonTrivialDtor::live = 0;

struct NonTrivialCopy
{
  int v;
  NonTrivialCopy (int v) : v (v) {}
  NonTrivialCopy (const NonTrivialCopy &o) : v (o.v) {}
};

static int
scalar_capture (const int x) post[old = x] (note (old))
{
  return x;
}

static int
trivial_dtor_capture (const int x) post[old = TrivialDtor{ x }] (note (old.v))
{
  return x;
}

static int
non_trivial_dtor_capture (const int x)
    post[old = NonTrivialDtor (x)] (note (old.v))
{
  return x;
}

static int
non_trivial_copy_capture (const int x)
    post[old = NonTrivialCopy (x)] (note (old.v))
{
  return x;
}

static int
several_captures (const int x) post[a = x, b = x + 1] (note (a + b))
{
  return x;
}

// The virtual capture struct is the second, separately lowered site.
struct Virtual
{
  virtual int
  f (const int x) post[old = x] (note (old))
  {
    return x;
  }
  virtual ~Virtual () = default;
};

// A capture whose value the body then changes, so the transcript cannot be
// satisfied by reading the parameter at postcondition time.
static int
capture_is_a_snapshot (const int x) post[old = x] (note (old))
{
  return x + 100;
}

int
main ()
{
  observed = 0;
  if (scalar_capture (4) != 4 || observed != 4)
    __builtin_abort ();

  observed = 0;
  if (trivial_dtor_capture (5) != 5 || observed != 5)
    __builtin_abort ();

  observed = 0;
  if (non_trivial_dtor_capture (6) != 6 || observed != 6)
    __builtin_abort ();
  if (NonTrivialDtor::live != 0) // the capture was destroyed
    __builtin_abort ();

  observed = 0;
  if (non_trivial_copy_capture (7) != 7 || observed != 7)
    __builtin_abort ();

  observed = 0;
  if (several_captures (8) != 8 || observed != 17)
    __builtin_abort ();

  {
    Virtual v;
    Virtual *p = &v;
    observed = 0;
    if (p->f (9) != 9 || observed != 9)
      __builtin_abort ();
  }

  observed = 0;
  if (capture_is_a_snapshot (10) != 110 || observed != 10)
    __builtin_abort ();

  // None of the above violates anything.
  if (violations != 0)
    __builtin_abort ();

  return 0;
}
