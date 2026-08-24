// P3100: the flow-off check appears in TWO places for a function-try-block.
// When the try body falls off, the handler catches the (observe) throw; if the
// handler then runs off its own end without returning, that is again flow-off
// UB, guarded by the check after the whole construct -- which throws and
// propagates out of f (no enclosing handler catches it).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-fntryblock-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

// observe.  For x<=0: try body falls off -> inside check throws -> caught by
// this handler -> handler falls off its end -> after-construct check throws ->
// propagates out of f.
int f (int x) try { if (x > 0) return x; } catch (E&) { /* no return */ }

int main () {
  bool caught = false;
  try { f (-1); } catch (E&) { caught = true; }
  if (!caught) __builtin_abort ();
  if (calls != 2) __builtin_abort ();   // inside check + after-construct check
}
