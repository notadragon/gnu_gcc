// D4405R0, "Previous Assertions and Undefined Behavior" (GCC-1003): a
// function's own precondition whose predicate calls a function is a premise
// for its body.  A proof resting on it is conditional -- not diagnosed,
// never assumed -- unless every function the predicate calls is found free
// of side effects, terminating and not throwing; then the premise's own
// classification decides, and an assume precondition's is guaranteed.
// Every precondition here is assume, every body evaluation prove, and each
// body evaluation can be proven only from its function's precondition.
//
// * sound_accessor: an inline accessor, pure in fact (not declared so):
//   sound, and assumed in the real code (-fcontracts-prove-assume).
// * cond_checked: use1's shape, the accessor has a precondition of its
//   own: conditional, not assumed.  A function with a contract assertion is
//   a refinable body (another translation unit's copy may evaluate the
//   assertion with observe, whose violation handler may have side effects),
//   so it is never found pure.
// * cond_opaque: a function only declared here: conditional, not assumed.
// * cond_effects: a callee that stores to a global: conditional.
// * cond_throws: a callee that may throw: conditional.
// * own_effect: a predicate with a side effect of its own (a compound
//   assignment through a mutable member) is not assumed at all, so the body
//   evaluation is not proven.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=assume -fcontracts-prove-assume" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven" }

#include <contracts>
using namespace std::contracts::labels;

int sink (int);
bool opaque (const int *);
int counter;

struct C {
  int *p;
  int *raw () const { return p; }
  int *get () const pre (p) { return p; }
  int *bump () const { ++counter; return p; }
  int *thr () const { if (!p) throw 1; return p; }
};

int sound_accessor (C &c) pre (c.raw () != nullptr)
{
  contract_assert<prove> (c.p != nullptr);
  return sink (*c.p);
}

int cond_checked (C &c) pre (c.get () != nullptr)
{
  contract_assert<prove> (c.p != nullptr);	// { dg-bogus "cannot be proven" }
  return sink (*c.p);
}

int cond_opaque (C &c) pre (opaque (c.p) && c.p != nullptr)
{
  contract_assert<prove> (c.p != nullptr);	// { dg-bogus "cannot be proven" }
  return sink (*c.p);
}

int cond_effects (C &c) pre (c.bump () != nullptr)
{
  contract_assert<prove> (c.p != nullptr);	// { dg-bogus "cannot be proven" }
  return sink (*c.p);
}

int cond_throws (C &c) pre (c.thr () != nullptr)
{
  contract_assert<prove> (c.p != nullptr);	// { dg-bogus "cannot be proven" }
  return sink (*c.p);
}

struct M { mutable int k; };

int own_effect (const M &m) pre ((m.k += 0) > 0)
{
  contract_assert<prove> (m.k > 0);	// { dg-error "cannot be proven" }
  return sink (m.k);
}

// { dg-final { scan-tree-dump {contract obligation 0 \(line 45\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 1 \(line 51\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 2 \(line 57\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 3 \(line 63\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 4 \(line 69\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 5 \(line 77\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 0: assumed} "contract_proven" } }
// { dg-final { scan-tree-dump-times {: assumed} 1 "contract_proven" } }
