// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-semantic=address:quick_enforce -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 4.1 REVISES Task 1.3 for the routed "address" check: quick_enforce
// means "terminate on the ASan error WITHOUT calling the contract handler",
// which AddressSanitizer can always do (it does not need -fsanitize-trap).  So
// -fsanitize-semantic=address:quick_enforce is now accepted (it was previously
// a hard error), and lowers to quick_enforce.
// { dg-regexp "address: quick_enforce" }
