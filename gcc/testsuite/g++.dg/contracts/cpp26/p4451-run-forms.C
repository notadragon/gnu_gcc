// P4451: a terminating contract assertion is enforced under a configured
// ignore or observe in the forms p4451-run.C does not cover: after a
// trailing return type, with a deduced return type, on a class template's
// members (defined in and out of class), a generic lambda, a function
// template with a deduced return type, and a contract_assert in a function
// template.  Run under -fcontracts-p4451 alone with ignore; the -p3400 and
// -both files include this one.
//
// Mirror: tests/tests/contracts/p4451/run_forms.sft.cpp in the
// edgcpp_compiler fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int handled;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != evaluation_semantic::enforce)
    std::abort ();
  ++handled;
  throw handled;
}

auto tr (int x) -> int post<terminating> (r : r > 0) { return x; }
auto ded (int x) post<terminating> (r : r > 0) { return x; }

template<class T> struct CT
{
  T m (T x) pre<terminating> (x > 0) { return x; }
  T d (T x) pre<terminating> (x > 0);
};
template<class T> T CT<T>::d (T x) pre<terminating> (x > 0) { return x; }

auto glam = [] (auto x) pre<terminating> (x > 0) { return x; };
template<class T> auto tded (T x) post<terminating> (r : r > 0) { return x; }
template<class T> void tassert (T x) { contract_assert<terminating> (x > 0); }

#define CHECK(E) \
  do { try { (void) (E); std::abort (); } catch (int) { } } while (0)

int
main ()
{
  CHECK (tr (-1));
  CHECK (ded (-1));
  CT<int> c;
  CHECK (c.m (-1));
  CHECK (c.d (-1));
  CHECK (glam (-1));
  CHECK (glam (-1L));
  CHECK (tded (-1));
  CHECK ((tassert (-1), 0));
  if (handled != 8)
    std::abort ();
  tr (1); ded (1); c.m (1); c.d (1); glam (1); tded (1); tassert (1);
  return handled == 8 ? 0 : 1;
}
