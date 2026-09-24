// P3100 x templates: the assume semantic on a function template with a
// dependent predicate.  assume currently lowers to ignore, so the (dependent)
// predicate is not evaluated for any instantiation -- verified via a
// side-effecting predicate helper.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int side = 0;
template <typename T>
bool pos(T x) { ++side; return x > T{}; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template <typename T>
T f(T x) pre(pos(x)) { return x; }

int main() {
  side = 0;
  violation_count = 0;

  // assume -> ignore: predicate not evaluated, so pos() never runs.
  if (f(-1) != -1) __builtin_abort();      // int instantiation
  if (f(-1.0) != -1.0) __builtin_abort();  // double instantiation

  if (side != 0) __builtin_abort();
  if (violation_count != 0) __builtin_abort();
}
