// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-semantic=address:quick_enforce -fsanitize-semantic-print" }
int main() { return 0; }
// The per-bit rule for the routed "address" check: quick_enforce
// means "terminate on the ASan error WITHOUT calling the contract handler",
// which AddressSanitizer can always do (it does not need -fsanitize-trap).  So
// -fsanitize-semantic=address:quick_enforce is accepted -- unlike a check that
// needs a trap to terminate -- and lowers to quick_enforce.
// { dg-regexp "address: quick_enforce" }
