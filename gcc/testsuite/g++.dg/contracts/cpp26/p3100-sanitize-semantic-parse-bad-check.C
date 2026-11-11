// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize-semantic=bogus:observe" }
int main() { return 0; }
// { dg-error "unknown sanitizer check" "" { target *-*-* } 0 }
