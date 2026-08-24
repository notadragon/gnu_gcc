// P3100: coroutine flow-off-end with noexcept_observe (-fcontracts-p4298) --
// the nonthrowing handler runs (assertion_kind::implicit) and control then
// proceeds to the final suspend.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-coro-flow-off-noexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <coroutine>
#include <contracts>

namespace cs = std::contracts;
static int calls = 0;
static bool ok = true;
void handle_contract_violation (const cs::contract_violation &v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit
      || v.semantic () != cs::evaluation_semantic::noexcept_observe)
    ok = false;
}

struct Task {
  struct promise_type {
    Task get_return_object () { return {}; }
    std::suspend_never initial_suspend () { return {}; }
    std::suspend_never final_suspend () noexcept { return {}; }
    void return_value (int) {}
    void unhandled_exception () {}
  };
};

namespace nxo_ns { Task foo (bool b) { if (b) co_return 1; } }   // noexcept_observe

int main () {
  nxo_ns::foo (false);
  if (calls != 1 || !ok) __builtin_abort ();
}
