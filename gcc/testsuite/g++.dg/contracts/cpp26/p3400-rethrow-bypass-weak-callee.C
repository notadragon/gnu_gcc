// P3400: the rethrowing-local-handler bypass does not trust the body of a weak
// callee, which the linker may replace.  Here the weak rethrow_helper is
// `throw;', but the strong definition in p3400-rethrow-bypass-weak-callee-2.cc
// logs first; the handler must run and see it.
//
// Clang: clang/test/Contracts/p3400-rethrow-bypass-weak-callee.cpp
// in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-require-weak "" }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-additional-sources "p3400-rethrow-bypass-weak-callee-2.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;

int logged = 0;
[[gnu::weak, noreturn]] void rethrow_helper () { throw; }

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation (const contract_violation&) const {
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
