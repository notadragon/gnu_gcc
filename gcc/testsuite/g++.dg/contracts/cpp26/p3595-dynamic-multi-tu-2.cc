// Second TU for p3595-dynamic-multi-tu.C: a strong definition of the selector
// that overrides the weak default emitted alongside the contract.  (Not a test
// on its own -- the lowercase .cc suffix keeps the g++.dg harness from
// collecting it.)

#include <contracts>

std::contracts::evaluation_semantic p3595_mt_sel() {
  return std::contracts::evaluation_semantic::observe;
}
