// The "unknown contract evaluation semantic" error for -fsanitize-semantic=
// quotes the whole name, however long.  The argument below is 70
// characters.
//
// Mirror: clang/test/Contracts/sanitize-semantic-long-name.cpp.
// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-semantic=address:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxABCDEFGHIJ" }
int main () { return 0; }
// { dg-error "unknown contract evaluation semantic .x+ABCDEFGHIJ." "" { target *-*-* } 0 }
