// P3100: control flowing off the end of a coroutine whose promise type has no
// usable return_void is core-language UB ({stmt.return.coroutine.flow.off}).
// observe reports (assertion_kind::implicit) then proceeds to the final suspend;
// ignore proceeds without reporting; a co_return avoids the fall-off entirely.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-coro-flow-off.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <coroutine>
#include <contracts>

namespace cs = std::contracts;
static int calls = 0;
static bool all_implicit = true;
static const char *last_comment = nullptr;
void handle_contract_violation (const cs::contract_violation &v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
  last_comment = v.comment ();
}

struct Task {
  struct promise_type {
    Task get_return_object () { return {}; }
    std::suspend_never initial_suspend () { return {}; }
    std::suspend_never final_suspend () noexcept { return {}; }
    void return_value (int) {}          // has return_value, NO return_void
    void unhandled_exception () {}
  };
};

namespace obs_ns { Task foo (bool b) { if (b) co_return 1; } }   // observe
namespace ign_ns { Task foo (bool b) { if (b) co_return 1; } }   // ignore

int main () {
  // observe: falls off the end -> handler reports, then proceeds to final
  // suspend (defined continuation; the return object stands).
  obs_ns::foo (false);
  if (calls != 1 || !all_implicit) __builtin_abort ();
  if (__builtin_strcmp (last_comment,
			"control flowed off the end of a coroutine") != 0)
    __builtin_abort ();

  // co_return: no fall-off, no report.
  obs_ns::foo (true);
  if (calls != 1) __builtin_abort ();

  // ignore: falls off but the assertion is dropped (defined continue, no report).
  ign_ns::foo (false);
  if (calls != 1) __builtin_abort ();
}
