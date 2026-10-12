// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-semantic=address:bogus" }
int main() { return 0; }
// { dg-error "unknown contract evaluation semantic" "" { target *-*-* } 0 }
