// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 4.1: with -fcontracts-p4298, the default -fsanitize=address (no
// -fsanitize-recover=) lowers the routed "address" check to noexcept_enforce
// (call the handler, then terminate).
// { dg-regexp "address: noexcept_enforce" }
