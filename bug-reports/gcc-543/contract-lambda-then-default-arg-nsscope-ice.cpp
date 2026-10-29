// g++ -std=c++26 -fcontracts -fsyntax-only contract-lambda-then-default-arg-nsscope-ice.cpp
//
// The namespace-scope form of contract-lambda-then-default-arg-ice.cpp.
// Stock GCC parses this predicate eagerly and compiles it; the contracts
// branch defers every predicate and crashes here too.

void f0 (int x) pre ([] { return true; } ());

void f4 (int y = 1);
