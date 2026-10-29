// P3100: a member function called in a base-class mem-initializer (ub:class.base.init.mem.fun.nonpolymorphic.mem.init)
// configured to "noexcept_enforce": the handler runs (kind=7, sem=7), then the program terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic.mem.init:noexcept_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce reports then terminates" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
struct Base { int v; Base (int y) : v (y) { } };
struct Derived : Base {
  int j;
  int f () const { return 7; }	// reads no member
  Derived () : Base (f ()), j (5) { }
};
int main () {
  Derived d;
  std::printf ("RESULT=%d\n", d.v == 7);
  return 0;
}
// { dg-output "VIOL kind=7 sem=7 comment=\\\[member function, typeid or dynamic_cast used before the bases are initialized or after they are destroyed\\\]" }
