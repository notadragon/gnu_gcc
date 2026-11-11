// P3400: the rethrowing-local-handler bypass accounts for the contracts of a
// callee it follows.  Calling `rethrow_helper ()' evaluates its
// `pre (log_it ())', which is observable, so the try/catch around the
// predicate stays.
//
// Clang: clang/test/Contracts/p3400-rethrow-bypass-callee-precondition.cpp
// in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

static int logged = 0;
bool log_it () { ++logged; return true; }
[[noreturn]] void rethrow_helper () pre (log_it ()) { throw; }

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      rethrow_helper ();
  }
};
constexpr rethrowing_t rethrowing{};

bool boom () { throw 42; }
int f (int i) pre<rethrowing> (boom ()) { return i; }

int main () {
  try { f (1); } catch (int) {}
  if (logged != 1)
    __builtin_abort ();
}
