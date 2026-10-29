// P3595, DECISIONS.md E137 and E138: caller-side dynamic selection under a
// label whose allowed set holds no throwing semantic (neither observe nor
// enforce): with -fcontracts-p4298 an invalid selection is reported at the
// call site, without evaluating the (true) predicate, as a noexcept_enforce
// violation rather than an enforce one.  The violation handler throws; the
// program must terminate, never reach the catch.  The label allows ignore
// so that the callee's own check, under the command line's ignore, stays
// inert.  Without -fcontracts-p4298 it traps (quick_enforce):
// p3595-caller-dyn-nothrow-label-no-p4298.C.
// Mirror: none (Clang has no caller-side checking).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-nothrow-label.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdlib>
#include <exception>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct nothrow_t {
  using assertion_control_object = nothrow_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::ignore, evaluation_semantic::noexcept_enforce,
     evaluation_semantic::quick_enforce};
};
constexpr nothrow_t nothrow_label{};

// Not a semantic: an invalid selection.
evaluation_semantic
p3595_caller_nothrow_sel ()
{
  return evaluation_semantic (42);
}

static int handled;
static evaluation_semantic seen;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++handled;
  seen = v.semantic ();
  throw 42;
}

static void
on_terminate ()
{
  if (handled == 1 && seen == evaluation_semantic::noexcept_enforce)
    std::_Exit (0);
  std::abort ();
}

int f (int x) pre<nothrow_label> (x > 0) { return x; }	// callee-side ignored
int call () { return f (1); }				// caller-side wrapper

int main ()
{
  std::set_terminate (on_terminate);
  try
    {
      call ();
    }
  catch (...)
    {
      std::abort ();	// no exception may leave the check
    }
  std::abort ();
}
