// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts -fcontracts-p3100 -fsanitize=undefined -fsanitize-semantic=undefined:prove" }
int main () { return 0; }
// P4405: without -fcontracts-p4405, -fsanitize-semantic= rejects prove for a
// group too: no member of "undefined" takes it.
//
// Mirror: clang/test/Contracts/p4405-sanitize-semantic.cpp in the
// llvm_llvm-project fork (its third RUN line).
// { dg-error ".-fsanitize-semantic=undefined:prove. is not supported: no members of .undefined. support .prove." "" { target *-*-* } 0 }
