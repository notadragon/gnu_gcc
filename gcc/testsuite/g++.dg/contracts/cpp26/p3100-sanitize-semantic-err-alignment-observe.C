// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=alignment -fsanitize-semantic=alignment:observe" }
int main() { return 0; }
// P3100 Task 4.1: as with enforce, plain (throwing) "observe" is not valid for
// a routed check; use noexcept_observe (with -fcontracts-p4298).
// { dg-error ".-fsanitize-semantic=alignment:observe. is not supported for a routed sanitizer check .a throwing handler cannot propagate.; use .noexcept_observe. with .-fcontracts-p4298." "" { target *-*-* } 0 }
