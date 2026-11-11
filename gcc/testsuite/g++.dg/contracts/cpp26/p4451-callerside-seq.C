// D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180, GCC-966): f has a
// terminating precondition #1 and an ordinary precondition #2.  A
// caller-side check of #2 may be made only if #1 is also checked at the
// call: the ignore that a caller-side evaluation adds to its allowed set
// means "do not duplicate this evaluation", which cannot be used for #1
// once #2 is checked caller-side.  So a P3595 configuration asking for #2
// caller-side makes #1 caller-side too.
//
// The paper's table of sequences, by the callee-side configuration (e1:
// #2 ignored in f; e12: both checked) and the caller-side one (c0: none;
// c1: #1; c2: #2; c12: both), each caller asking with the observe semantic,
// which #1, terminating, evaluates as enforce.  The sequences made, caller
// side then callee side:
//   c0:  #1        and #1 #2
//   c1:  #1 #1     and #1 #1 #2
//   c2:  #1 #2 #1  and #1 #2 #1 #2   (#1 made caller-side because of #2)
//   c12: as c2
// Each call is also made failing each of its checks in turn; the
// violation handler throws, so the sequence stops there.  Under
// -fcontracts-p4451 alone; p4451-callerside-seq-p3400.C and
// p4451-callerside-seq-both.C include this file.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-seq.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

static char trace[32];
static int evaluations, target, last_id;

static bool
check (int id)
{
  std::size_t n = std::strlen (trace);
  trace[n] = '0' + id;
  trace[n + 1] = '\0';
  last_id = id;
  return ++evaluations != target;
}

struct violation {};

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (last_id == 1
      && v.semantic () != std::contracts::evaluation_semantic::enforce)
    std::abort ();
  std::strcat (trace, "v");
  throw violation {};
}

namespace e1 {
void f ()
  pre<terminating> (check (1))	// line 58
  pre (check (2));		// line 59: ignored callee-side
}

namespace e12 {
void f ()
  pre<terminating> (check (1))	// line 64
  pre (check (2));		// line 65
}

void e1::f () { }
void e12::f () { }

namespace c0 { void g1 () { e1::f (); } void g12 () { e12::f (); } }
namespace c1 { void g1 () { e1::f (); } void g12 () { e12::f (); } }
namespace c2 { void g1 () { e1::f (); } void g12 () { e12::f (); } }
namespace c12 { void g1 () { e1::f (); } void g12 () { e12::f (); } }

// Call G with no violation, expecting the checks SEQ, then once failing
// each check in turn.
static void
run (void (*g) (), const char *seq)
{
  for (int n = 0; n <= (int) std::strlen (seq); ++n)
    {
      trace[0] = '\0';
      evaluations = 0;
      target = n;
      bool threw = false;
      try { g (); } catch (violation) { threw = true; }
      char expected[32];
      std::strcpy (expected, seq);
      if (n)
	{
	  expected[n] = 'v';
	  expected[n + 1] = '\0';
	}
      if (threw != (n != 0) || std::strcmp (trace, expected) != 0)
	std::abort ();
    }
}

int
main ()
{
  run (c0::g1, "1");
  run (c0::g12, "12");
  run (c1::g1, "11");
  run (c1::g12, "112");
  run (c2::g1, "121");
  run (c2::g12, "1212");
  run (c12::g1, "121");
  run (c12::g12, "1212");
}
