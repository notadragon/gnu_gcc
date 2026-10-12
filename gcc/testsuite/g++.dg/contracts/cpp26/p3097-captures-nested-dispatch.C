// P3097+P3098: Nested virtual dispatch where both levels have capturing
// postconditions.  A capturing virtual A::f, during its body, makes a second
// capturing virtual call B::g; verify both capture sets are established and
// checked independently and both violations are reported.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int cA = 0;
static int cB = 0;

struct BaseB {
  virtual int g()
    post [old_b = cB] (r: r == old_b + 1)
  { return ++cB; }
  virtual ~BaseB() = default;
};

struct DerivedB : BaseB {
  int g() override { return cB += 2; }  // disagrees with BaseB post (+1)
};

struct BaseA {
  virtual int f(BaseB& b)
    post [old_a = cA] (r: r == old_a + 1)
  { b.g(); return ++cA; }
  virtual ~BaseA() = default;
};

struct DerivedA : BaseA {
  int f(BaseB& b) override
  {
    b.g();          // nested virtual dispatch, violates BaseB interface post
    return cA += 2; // disagrees with BaseA post (+1)
  }
};

int main() {
  DerivedA da;
  DerivedB db;
  BaseA& a = da;
  BaseB& b = db;

  cA = 0;
  cB = 0;
  violation_count = 0;

  // a.f(b): BaseA captures old_a = 0.  DerivedA::f body calls b.g():
  //   BaseB captures old_b = 0, DerivedB::g sets cB = 2 -> 2 == 0+1 false (viol 1).
  // DerivedA::f returns cA = 2 -> BaseA post 2 == 0+1 false (viol 2).
  if (a.f(b) != 2) __builtin_abort();
  if (violation_count != 2) __builtin_abort();
}
