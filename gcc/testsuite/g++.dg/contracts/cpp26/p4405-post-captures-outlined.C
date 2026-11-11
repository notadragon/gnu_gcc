// P4405: a postcondition with captures (P3098) under prove or diagnose
// initializes no captures -- a capture initializer that would throw is never
// run -- and its predicate is not evaluated; the evaluation is diagnosed
// where its check would be emitted, here in the outlined check function.
//
// GCC-only: -fcontract-checks-outlined has no Clang counterpart
// (notadragon_wg21 DECISIONS.md B6); p4405-post-captures.C is the mirrored
// form.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int captured, evaluated, violations;
int cap (int x) { ++captured; if (x < 0) throw 1; return x; }
bool chk (int c) { ++evaluated; return c > 0; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

struct S { int v; S (int x) : v (x) {} ~S () {} };

int f (int x) post [c = cap (x)] (chk (c)) { return x; }	// { dg-warning "contract assertion cannot be proven" }
S g (int x) post [s = S (cap (x))] (r: chk (r.v + s.v)) { return S (x); } // { dg-warning "contract assertion cannot be proven" }

int main ()
{
  f (-1);
  g (-1);
  if (captured != 0 || evaluated != 0 || violations != 0)
    __builtin_abort ();
}
