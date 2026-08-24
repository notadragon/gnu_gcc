// P3595 x config: "kind" matching selects semantics per contract kind.  Only
// kind:"pre" was previously covered; this exercises kind:"post" and
// kind:"assert".  The config sends post and assert to ignore and lets pre fall
// through to the observe catch-all.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-kind.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

void has_pre(int x) pre(x > 0) { }                 // catch-all -> observe
int has_post(int x) post(r: r > 0) { return x; }   // kind:post -> ignore
void has_assert(int x) { contract_assert(x > 0); } // kind:assert -> ignore

int main() {
  has_pre(-1);      // observe -> handler called
  if (violations != 1) __builtin_abort();
  has_post(-1);     // ignore -> no handler
  if (violations != 1) __builtin_abort();
  has_assert(-1);   // ignore -> no handler
  if (violations != 1) __builtin_abort();
}
