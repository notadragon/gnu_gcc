// D4298: noexcept_enforce/noexcept_observe are only valid evaluation
// semantics when -fcontracts-p4298 is enabled.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce" }
int f(int x) pre(x > 0) { return x; }
int main() { return f(1); }
