// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-recover=address -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 1.2 + Task 4.1: with no explicit -fsanitize-semantic= entry, the
// routed "address" check with -fsanitize-recover= set lowers to the
// noexcept_observe contract evaluation semantic (call the handler, then
// continue).  This requires -fcontracts-p4298 -- without it the combination is
// a hard error (see p3100-sanitize-semantic-recover-no-p4298.C), because a
// throwing handler cannot propagate from a routed sanitizer check and there is
// no other non-throwing way to honor continue-on-violation.
// { dg-regexp "address: noexcept_observe" }
