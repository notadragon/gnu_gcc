// P3400: the rethrowing-local-handler bypass does not treat a read of a
// volatile object as pure.  A volatile access is observable behaviour
// ([intro.abstract]), so the handler below -- `touched; throw;' -- must run,
// and the predicate needs its catch.  (With the read removed the bypass does
// drop the catch, so the analysis is active here and refuses for the right
// reason.)
//
// Clang: clang/test/Contracts/p3400-rethrow-bypass-volatile-read.cpp
// in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe -O0" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

volatile int touched = 0;

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      {
	touched;
	throw;
      }
  }
};
constexpr rethrowing_t rethrowing{};

bool boom () { throw 42; }
int f (int i) pre<rethrowing> (boom ()) { return i; }

// { dg-final { scan-assembler "__cxa_begin_catch" } }
