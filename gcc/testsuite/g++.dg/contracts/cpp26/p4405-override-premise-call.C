// D4405R0, "Previous Assertions and Undefined Behavior" and "Virtual
// Functions" (GCC-1003): an overridden function's precondition whose
// predicate calls a function is a premise in an override shadow, as a
// function's own precondition is in its proof shadow
// (p4405-own-pre-call-premise-sound.C).  Each B precondition is
// always_enforce, so guaranteed (p4405-override-premise.C); the proof of
// D's precondition resting on it is sound if every function the predicate
// calls is found free of side effects, terminating and not throwing (g: an
// inline function), and conditional otherwise (h: a function only declared
// here; k: an inline function with a precondition of its own, a refinable
// body, so never found pure).  None is diagnosed.
// Mirror: clang/test/Contracts/p4405-override-premise-call.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
using namespace std::contracts::labels;

bool opaque (int);
inline bool positive (int x) { return x > 0; }
inline bool checked (int x) pre (x != 5) { return x > 0; }

struct B {
  virtual int g (int x) pre<always_enforce> (positive (x));
  virtual int h (int x) pre<always_enforce> (opaque (x) && x > 0);
  virtual int k (int x) pre<always_enforce> (checked (x));
};

struct D : B {
  int g (int x) override pre (x >= 1) { return x; }	// { dg-bogus "cannot be proven" }
  int h (int x) override pre (x >= 1) { return x; }	// { dg-bogus "cannot be proven" }
  int k (int x) override pre (x >= 1) { return x; }	// { dg-bogus "cannot be proven" }
};

void use () { D d; }

// { dg-final { scan-tree-dump {\(line 32, override pair [0-9]+\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 33, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 34, override pair [0-9]+\): conditional} "contract_shadow" } }
