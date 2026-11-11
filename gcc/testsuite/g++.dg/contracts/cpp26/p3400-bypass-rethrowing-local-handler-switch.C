// P3400: a local violation handler that rethrows an evaluation_exception
// from a switch on detection_mode () has its try/catch elided
// (p3400-bypass-rethrowing-local-handler-switch-codegen.C); behaviour is
// unchanged: the exception escapes the guarded function, the global handler
// never sees it, and a false predicate still takes the switch's other arm.
//
// Mirror: clang/test/Contracts/p3400-bypass-rethrowing-local-handler-switch.cpp
// in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

static int global_calls, local_false;

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation (const contract_violation& v) const {
    switch (v.detection_mode ())
      {
      case detection_mode::evaluation_exception:
	throw;
      case detection_mode::predicate_false:
	++local_false;
	break;
      default:
	break;
      }
  }
};
constexpr rethrowing_t rethrowing{};

void handle_contract_violation (const contract_violation&) { ++global_calls; }

struct predicate_error { int tag; };
static bool boom () { throw predicate_error{ 7 }; }

int f (int i) pre<rethrowing> (boom ()) { return i; }
int g (int i) pre<rethrowing> (i > 0) { return i; }

int main ()
{
  int caught = 0;
  try { f (1); } catch (const predicate_error& e) { caught = e.tag; }
  if (caught != 7 || global_calls != 0)
    __builtin_abort ();
  g (-1);
  if (local_false != 1 || global_calls != 1)
    __builtin_abort ();
}
