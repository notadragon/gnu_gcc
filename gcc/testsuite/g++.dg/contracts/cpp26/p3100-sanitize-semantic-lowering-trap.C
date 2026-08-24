// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=shift -fsanitize-trap=shift -fsanitize-semantic-print" }
int main() { return 0; }
// With no explicit -fsanitize-semantic= entry, a trappable check with
// -fsanitize-trap= set lowers to the "quick_enforce" contract
// evaluation semantic (P3100 Task 1.2).
// { dg-regexp "shift: quick_enforce" }
