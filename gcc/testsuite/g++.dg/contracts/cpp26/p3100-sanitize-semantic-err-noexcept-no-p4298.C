// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-semantic=address:noexcept_enforce" }
int main() { return 0; }
// P3100 Task 4.1: the D4298 noexcept_enforce/noexcept_observe semantics for the
// routed "address" check require -fcontracts-p4298, exactly like the ordinary-
// contract path.  Requesting one without that flag is a hard error naming it.
// { dg-error ".-fsanitize-semantic=address:noexcept_enforce. requires .-fcontracts-p4298." "" { target *-*-* } 0 }
