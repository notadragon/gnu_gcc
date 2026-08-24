// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=shift -fsanitize-semantic=shift-base:quick_enforce -fsanitize-semantic-print" }
int main() { return 0; }
// "shift-base" is a routed UBSan check and quick_enforce is always in a routed
// check's allowed set: setting it on this multi-bit-exclusive sub-bit must
// compile clean and store the value (P3100 Task 1.3, positive multi-bit case).
// -fcontracts-p4298 is required because -fsanitize=shift also enables the
// sibling shift-exponent, which (recover-by-default) derives noexcept_observe --
// a routed continuing semantic that needs the nonthrowing-semantics flag.
// { dg-regexp "shift: quick_enforce" }
