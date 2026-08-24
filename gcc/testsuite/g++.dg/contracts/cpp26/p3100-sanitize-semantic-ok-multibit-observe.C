// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=shift -fsanitize-semantic=shift-exponent:noexcept_observe -fsanitize-semantic-print" }
int main() { return 0; }
// "shift-exponent" is a routed UBSan check (full-coverage routing), so its
// continuing semantic is the non-throwing noexcept_observe (plain "observe" is
// a hard error for a routed check).  Requesting it on the multi-bit-exclusive
// sub-bit must compile clean and store the value, visible for the whole "shift"
// group via the debug seam (P3100 Task 1.3 allowed-set, positive case).
// { dg-regexp "shift: noexcept_observe" }
