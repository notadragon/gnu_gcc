// The -fsanitize-semantic-print debug seam prints a check only when it is
// enabled: -fsanitize=kernel-address sets the family bit it shares with
// "address", but enables no user-space address check, so nothing is printed
// (a stray line would be excess output).
//
// Mirror: clang/test/Contracts/sanitize-semantic-print-kernel-address.cpp.
// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=kernel-address -fsanitize-semantic-print" }
int main () { return 0; }
