// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize=pointer-compare -fsanitize-semantic=pointer-compare:quick_enforce -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 4.1: the ASan pointer-compare check ([expr.rel] UB) is routed, so
// it takes the routed allowed set.  quick_enforce means "terminate WITHOUT
// calling the contract handler", always available for a routed check, so
// -fsanitize-semantic=pointer-compare:quick_enforce is accepted and lowers to
// quick_enforce.  (pointer-compare must be combined with -fsanitize=address.)
// The required -fsanitize=address also prints its own derived semantic line;
// consume it too so it is not counted as excess output.
// { dg-regexp "pointer-compare: quick_enforce" }
// { dg-regexp "address: quick_enforce" }
