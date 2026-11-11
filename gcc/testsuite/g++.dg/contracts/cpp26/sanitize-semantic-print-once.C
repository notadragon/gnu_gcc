// -fsanitize-semantic-print prints each check's semantic exactly once: the
// dg-regexp consumes one line, and a second would be excess output.  Clang
// printed it twice when its arguments made a round trip (CLANG-139).
//
// Mirror: clang/test/Contracts/sanitize-semantic-print-once.cpp in the
// llvm_llvm-project fork (GCC-668).
// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-semantic=address:noexcept_enforce -fsanitize-semantic-print" }
// { dg-regexp "address: noexcept_enforce\n" }
int main () { return 0; }
