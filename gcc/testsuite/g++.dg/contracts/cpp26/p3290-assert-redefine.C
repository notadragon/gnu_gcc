// P3290: <cassert> preserves the atypical assert-redefinition behavior -- a
// re-include with a different NDEBUG changes what `assert` expands to, even with
// the contract integration active.  Under NDEBUG the failed assert must be a
// no-op (handler not called); without NDEBUG it is active again.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cassert>
#include <cstdio>

static int hits = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++hits; }

#define NDEBUG
#include <cassert>
static void ndebug_call() { assert(1 == 2); }   // no-op under NDEBUG

#undef NDEBUG
#include <cassert>
static void active_call() { assert(1 == 1); }    // active again; passes

int main() {
  ndebug_call();
  if (hits != 0)
    __builtin_abort();       // NDEBUG assert must not have called the handler
  active_call();
  return 0;
}
