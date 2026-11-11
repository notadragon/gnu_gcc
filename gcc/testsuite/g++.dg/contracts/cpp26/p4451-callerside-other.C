// D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180, GCC-966): when a
// precondition is checked at the call, every earlier precondition that
// does not allow ignore (here a terminating one) is checked at the call
// too; one that allows ignore is not, nor is anything after the last
// precondition checked at the call, nor a postcondition (its caller-side
// check follows the callee's).  The configuration
// (p4451-callerside-other.json) asks for the caller-side checks of the
// lines marked "caller", with the observe semantic.  Sequences made,
// caller side then callee side:
//   a: #2 alone; #1, ordinary, is not made at the call: 2 12
//   b: #3; both earlier, terminating, are made at the call: 123 123
//   c: #1; the terminating #2 after it is not: 1 12
//   d: #3; #2, terminating, is made at the call, #1, ordinary, is not:
//      23 123
//   e: #1, the precondition; the terminating postcondition #2 is not made
//      at the call: 1 1 2
//   f: #2, the terminating postcondition, alone: 1 2 2
// Under -fcontracts-p4451 alone; the -p3400 and -both files include this
// one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-other.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

static char trace[32];

static bool
check (int id)
{
  std::size_t n = std::strlen (trace);
  trace[n] = '0' + id;
  trace[n + 1] = '\0';
  return true;
}

void a ()
  pre (check (1))
  pre<terminating> (check (2)) { }		// caller (line 42)

void b ()
  pre<terminating> (check (1))
  pre<terminating> (check (2))
  pre (check (3)) { }				// caller (line 47)

void c ()
  pre (check (1))				// caller (line 50)
  pre<terminating> (check (2)) { }

void d ()
  pre (check (1))
  pre<terminating> (check (2))
  pre (check (3)) { }				// caller (line 56)

void e ()
  pre (check (1))				// caller (line 59)
  post<terminating> (check (2)) { }

void f ()
  pre (check (1))
  post<terminating> (check (2)) { }		// caller (line 64)

static void
run (void (*g) (), const char *seq)
{
  trace[0] = '\0';
  g ();
  if (std::strcmp (trace, seq) != 0)
    std::abort ();
}

int
main ()
{
  run ([] { a (); }, "212");
  run ([] { b (); }, "123123");
  run ([] { c (); }, "112");
  run ([] { d (); }, "23123");
  run ([] { e (); }, "112");
  run ([] { f (); }, "122");
}
