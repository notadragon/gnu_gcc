// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-semantic=address:enforce" }
int main() { return 0; }
// P3100 Task 4.1: the routed "address" check invokes the contract handler from
// inside libasan's implicitly-noexcept ScopedInErrorReport destructor, so a
// throwing handler can never propagate.  Plain (throwing) "enforce" is therefore
// not a valid semantic for a routed check -- even with -fcontracts-p4298 -- and
// is a hard error suggesting the non-throwing alternatives.
// { dg-error ".-fsanitize-semantic=address:enforce. is not supported for a routed sanitizer check .a throwing handler cannot propagate.; use .noexcept_enforce." "" { target *-*-* } 0 }
