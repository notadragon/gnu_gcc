// P3595, DECISIONS.md E138: without -fcontracts-p4298, an invalid
// caller-side dynamic selection under a label whose allowed set holds no
// throwing semantic (neither observe nor enforce) traps at the call site as
// quick_enforce: the violation handler is not called and the (true)
// predicate is not evaluated.  The label allows ignore so that the callee's
// own check, under the command line's ignore, stays inert.  With
// -fcontracts-p4298: p3595-caller-dyn-nothrow-label.C.
// Mirror: none (Clang has no caller-side checking).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-nothrow-label-no-p4298.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce" }
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

// quick_enforce calls no handler: a call is a failure, and exiting normally
// is how a dg-shouldfail test reports one.  So is an exception reaching the
// catch, a terminate, or returning from f.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

[[noreturn]] static void
on_terminate ()
{
  std::_Exit (0);
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
      std::_Exit (0);
    }
  return 0;
}
