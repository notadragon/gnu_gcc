// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4405 -fsanitize=address,return -fsanitize-semantic=address:prove,return:diagnose" }
int main () { return 0; }
// P4405: -fsanitize-semantic= rejects prove and diagnose, for a routed check
// (address) and for one that cannot recover (return) alike: no P3100
// implicit assertion allows them.  The existing allowed-set check rejects
// them.  p4405-sanitize-semantic-no-flag.C is the same without
// -fcontracts-p4405, for a group.
//
// Mirror: clang/test/Contracts/p4405-sanitize-semantic.cpp in the
// llvm_llvm-project fork.
// { dg-error ".-fsanitize-semantic=address:prove. is not supported" "" { target *-*-* } 0 }
// { dg-error ".-fsanitize-semantic=return:diagnose. is not supported" "" { target *-*-* } 0 }
