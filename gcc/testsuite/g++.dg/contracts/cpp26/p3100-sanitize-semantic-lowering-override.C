// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-recover=address -fsanitize-semantic=address:noexcept_enforce -fsanitize-semantic-print" }
int main() { return 0; }
// An explicit -fsanitize-semantic= entry overrides the value that would
// otherwise be derived from -fsanitize-recover= (P3100 Task 1.2): the explicit
// noexcept_enforce wins over the recover-derived noexcept_observe.  Task 4.1:
// the routed "address" check accepts the noexcept_{enforce,observe} semantics
// (under -fcontracts-p4298), not plain throwing enforce/observe.
// { dg-regexp "address: noexcept_enforce" }
