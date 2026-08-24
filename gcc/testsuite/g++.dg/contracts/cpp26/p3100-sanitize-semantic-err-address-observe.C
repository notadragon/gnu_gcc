// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-semantic=address:observe" }
int main() { return 0; }
// P3100 Task 4.1: plain (throwing) "observe" is not a valid semantic for the
// routed "address" check -- a throwing handler cannot propagate from the
// libasan noexcept report path -- so it is a hard error suggesting
// noexcept_observe (which requires -fcontracts-p4298).
// { dg-error ".-fsanitize-semantic=address:observe. is not supported for a routed sanitizer check .a throwing handler cannot propagate.; use .noexcept_observe." "" { target *-*-* } 0 }
