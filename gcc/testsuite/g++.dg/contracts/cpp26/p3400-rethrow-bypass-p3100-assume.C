// P3400: the rethrowing-local-handler bypass does not look through a
// statement attribute with run-time effect.  Under -fcontracts-p3100 an
// `[[assume]]' is a checked implicit assertion; here it is false and
// configured to observe, so running the handler reports a violation before
// the rethrow and the try/catch around the predicate stays.
//
// Clang: clang/test/Contracts/p3400-rethrow-bypass-p3100-assume.cpp
// in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3400-rethrow-bypass-p3100-assume.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;

static int reported = 0;
void handle_contract_violation (const contract_violation&) { ++reported; }

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation (const contract_violation&) const {
    int k = 0;
    [[assume (k == 1)]];
    throw;
  }
};
constexpr rethrowing_t rethrowing{};

bool boom () { throw 42; }
int f (int i) pre<rethrowing> (boom ()) { return i; }

int main () {
  try { f (1); } catch (int) {}
  if (reported != 1)
    __builtin_abort ();
}
