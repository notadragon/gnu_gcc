// { dg-options "-fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
// { dg-do run { target c++26 } }

// P4301 contract_violation::report().  For a hand-written contract there is
// no populator field at all, so report() yields nullptr -- distinct from a
// populator that produced an empty string.  The default handler relies on
// exactly that distinction (contract26.cc guards with `__r && __r[0]`).
// It must also be noexcept and callable repeatedly.
//
// The producer-backed paths (ASan/UBSan/TSan) are covered from g++.dg,
// which can run under a sanitizer; what is checked here is the API contract
// itself, which had no libstdc++ coverage at all.

#include <contracts>
#include <testsuite_hooks.h>

static int calls = 0;

void handle_contract_violation(const std::contracts::contract_violation& v)
{
  ++calls;

  static_assert(noexcept(v.report()));

  // No populator for a hand-written contract.
  VERIFY( v.report() == nullptr );

  // Idempotent: the lookup is lazy, so a second call must not differ.
  VERIFY( v.report() == nullptr );
}

void f(int i) pre (i > 10) { }

int main()
{
  f(0);
  VERIFY( calls == 1 );
}
