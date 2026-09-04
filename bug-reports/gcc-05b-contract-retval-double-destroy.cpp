// gcc-05b-contract-retval-double-destroy.cpp                        -*-C++-*-
//
// GCC-5 (b): a contract on a function that returns a class by value and has
// a cleanup that might throw destroys the returned object TWICE when that
// cleanup does throw.  No diagnostic; the program simply runs a destructor
// on an object that has already been destroyed.
//
//   g++ -std=c++26 -fcontracts gcc-05b-contract-retval-double-destroy.cpp
//   ./a.out; echo $?
//
//     -> 255  (i.e. live == -1: one construction, two destructions)
//
// Deleting the `pre (true)` -- and nothing else -- prints 0.  The predicate's
// content is irrelevant; it is never false and never fails.  `post` behaves
// the same way, as does plain `-std=c++23 -fcontracts`.
//
// This is the same defect as (a), the ICE in
// `gcc-05-calendar-gimplify-ice.cpp`, seen from its other side: a contract
// makes the compiler treat one function body as two, so the machinery that
// destroys a by-value return object when a cleanup throws is emitted twice.
// Where the returned object is also a named return value, the duplicated
// declaration of the compiler's internal sentinel variable trips an assertion
// in the gimplifier and you get (a) instead; where it is not, there is no
// assertion to trip and the duplicated cleanup runs.
//
// (b) is the one to lead a report with: an ICE is obvious and stops the
// build, whereas this silently corrupts a program that compiles cleanly.

int live = 0;

struct Counted {
  int v;
  Counted (int x) : v (x) { ++live; }
  Counted (const Counted &o) : v (o.v) { ++live; }
  ~Counted () { --live; }
};

struct ThrowOnDestroy {
  bool armed;
  ~ThrowOnDestroy () noexcept (false) { if (armed) throw 42; }
};

Counted
f (bool arm) pre (true)          // delete `pre (true)' and the bug goes away
{
  ThrowOnDestroy guard { arm };
  Counted result (7);
  return result;                 // built, then `guard' throws on the way out
}

int
main ()
{
  try { f (true); } catch (int) { }
  return live;                   // 0 expected; -1 (exit 255) without the fix
}
