// P3100: a THROWING violation handler for a coroutine flow-off assertion.  The
// fall-off point is inside the coroutine's try block, so a throwing observe or
// enforce reaction unwinds the coroutine body (destroying in-scope automatic
// objects) and is caught by promise.unhandled_exception ().
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-coro-flow-off-throw.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <coroutine>
#include <contracts>

struct E {};
static int ueh = 0;
static int dtors = 0;
struct S { ~S () { ++dtors; } };
void handle_contract_violation (const std::contracts::contract_violation &) {
  throw E{};
}

struct Task {
  struct promise_type {
    Task get_return_object () { return {}; }
    std::suspend_never initial_suspend () { return {}; }
    std::suspend_never final_suspend () noexcept { return {}; }
    void return_value (int) {}
    void unhandled_exception () { ++ueh; }   // catches the thrown reaction
  };
};

namespace obs_ns { Task foo (bool b) { S s; if (b) co_return 1; } }   // observe
namespace enf_ns { Task foo (bool b) { S s; if (b) co_return 1; } }   // enforce

int main () {
  // Throwing observe: unwinds (S::~S runs), caught by unhandled_exception.
  obs_ns::foo (false);
  if (ueh != 1 || dtors != 1) __builtin_abort ();
  // Throwing enforce: same -- unwinds and is caught by unhandled_exception.
  enf_ns::foo (false);
  if (ueh != 2 || dtors != 2) __builtin_abort ();
}
