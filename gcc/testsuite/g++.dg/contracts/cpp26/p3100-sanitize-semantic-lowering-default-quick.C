// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 4.1: WITHOUT -fcontracts-p4298, the default -fsanitize=address
// (no -fsanitize-recover=) lowers the routed "address" check to quick_enforce
// (terminate on the ASan error without calling the handler).  This is the new
// default for a plain -fcontracts-p3100 -fsanitize=address build.
// { dg-regexp "address: quick_enforce" }
