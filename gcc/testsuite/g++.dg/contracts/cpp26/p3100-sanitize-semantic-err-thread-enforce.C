// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=thread -fsanitize-semantic=thread:enforce" }
int main() { return 0; }
// P3100 Task 4.1: the routed "thread" check invokes the contract handler from
// inside libtsan's implicitly-noexcept report path, so a throwing handler can
// never propagate.  Plain (throwing) "enforce" is therefore not a valid
// semantic for a routed check -- even with -fcontracts-p4298 -- and is a hard
// error suggesting the non-throwing alternatives.
// { dg-error ".-fsanitize-semantic=thread:enforce. is not supported for a routed sanitizer check .a throwing handler cannot propagate.; use .noexcept_enforce." "" { target *-*-* } 0 }
