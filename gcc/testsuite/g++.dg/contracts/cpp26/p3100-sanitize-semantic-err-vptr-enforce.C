// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fsanitize-semantic=vptr:enforce" }
int main() { return 0; }
// P3100 Task 4.1: vptr is a routed UBSan check -- its report is invoked from
// libubsan's implicitly-noexcept ScopedReport destructor, so a throwing handler
// can never propagate.  Plain (throwing) "enforce" is therefore not a valid
// semantic for a routed check, even with -fcontracts-p4298; it is a hard error
// suggesting the non-throwing alternatives.
// { dg-error ".-fsanitize-semantic=vptr:enforce. is not supported for a routed sanitizer check .a throwing handler cannot propagate.; use .noexcept_enforce." "" { target *-*-* } 0 }
