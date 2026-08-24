// P3097 x P3595: a virtual function's interface wrapper resolves its contract
// semantic using the CALLEE-side configuration.  The config here sets the
// callee side to ignore and the caller side to a dynamic selector returning
// observe; a polymorphic call whose interface precondition fails is NOT
// reported, because the wrapper uses the callee-side ignore and the caller-side
// dynamic selector is not consulted for the interface wrapper.
//
// (An additional caller-side check when calling into the interface is a possible
// future feature; it is intentionally not performed today.  Earlier drafts of
// p3595-overview.md described the wrapper as caller-side, which was incorrect.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3097-caller-dyn-virtual.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
using std::contracts::evaluation_semantic;

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++fired;
}

evaluation_semantic p3097_caller_sel() { return evaluation_semantic::observe; }

struct Base {
  virtual int f(int x) pre(x > 0) { return x; }
  virtual ~Base() = default;
};
struct Derived : Base {
  int f(int x) override pre(x > 0) { return x; }
};

int call(Base& b) { return b.f(-1); }   // polymorphic call site

int main() {
  Derived d;
  Base& b = d;
  call(b);
  // Callee-side is ignore -> no check; the caller-side dynamic selector is not
  // consulted for the virtual interface wrapper.
  if (fired != 0) __builtin_abort();
}
