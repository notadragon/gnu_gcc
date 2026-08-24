// P3100: coroutine flow-off-end with quick_enforce -- traps at the fall-off.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-coro-flow-off-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "coroutine flow-off quick_enforce" }

#include <coroutine>

struct Task {
  struct promise_type {
    Task get_return_object () { return {}; }
    std::suspend_never initial_suspend () { return {}; }
    std::suspend_never final_suspend () noexcept { return {}; }
    void return_value (int) {}
    void unhandled_exception () {}
  };
};

Task foo (bool b) { if (b) co_return 1; }

int main () { foo (false); return 0; }
