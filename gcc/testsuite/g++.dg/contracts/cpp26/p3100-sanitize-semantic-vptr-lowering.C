// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fsanitize-recover=vptr -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 1.2 + Task 4.1: with no explicit -fsanitize-semantic= entry, the
// routed vptr check with -fsanitize-recover= set lowers to the noexcept_observe
// contract evaluation semantic (call the handler, then continue).
// { dg-regexp "vptr: noexcept_observe" }
