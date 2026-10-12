// P3097: the full two-source evaluation order for a virtual call through a base
// reference is interface precondition, implementation precondition, the body,
// implementation postcondition, interface postcondition.  p3097-postcondition.C
// checks the impl-post-before-interface-post pair; this pins the *complete*
// sequence, including the precondition order and the body's position, by making
// all four contracts fail under observe and having the body log a marker into
// the same ordered log.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int idx = 0;
static const char* log[10];

void handle_contract_violation(const std::contracts::contract_violation& v) {
  if (idx < 10)
    log[idx++] = v.comment();
}

struct Base {
  virtual int f(int x)
    pre(x < 0)       // interface pre: fails for x = 5
    post(r: r < 0)   // interface post: fails
  { return x; }
};

struct Derived : Base {
  int f(int x) override
    pre(x > 100)     // implementation pre: fails for x = 5
    post(r: r > 100) // implementation post: fails
  {
    if (idx < 10)
      log[idx++] = "BODY";
    return x;
  }
};

int main() {
  Derived d;
  Base& b = d;
  b.f(5);

  const char* expected[] = {"x < 0", "x > 100", "BODY", "r > 100", "r < 0"};
  if (idx != 5)
    __builtin_abort();
  for (int i = 0; i < 5; ++i)
    if (std::strcmp(log[i], expected[i]) != 0)
      __builtin_abort();
}
