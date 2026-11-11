// P3098: constructor postconditions with captures, with the checks
// outlined.  The capture must be replaced by its capture-struct field before
// the parameters are remapped, and the call to the outlined functions must
// pass the capture struct after the __in_chrg / __vtt_parm arguments of a
// class with a virtual base.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

struct A {
  int v;
  A (int x) post [c = x] (c == v) : v (x) {}
};

struct V { int b; V (int x) : b (x) {} };
struct B : virtual V {
  int v;
  B (int x) post [c = x] (c == v) : V (x), v (x + 1) {}
};

struct C : B {
  C (int x) : V (x), B (x) {}
};

int main ()
{
  A a (1);
  if (violations != 0)
    __builtin_abort ();
  B b (1);			// c == 1, v == 2
  if (violations != 1)
    __builtin_abort ();
  C c (1);			// B's base-object constructor: the same check
  if (violations != 2)
    __builtin_abort ();
}
