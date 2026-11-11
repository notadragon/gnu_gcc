// P4405, notadragon_wg21 DECISIONS.md E196 item 1, at proof level 0: only
// folding runs, which takes no premise from an earlier evaluation, so each
// of the three preconditions is warned at the call (a proof shadow, at -O0
// too by default, gives one: p4405-premise-unproven.C).  Accepted: what
// level 0 catches is not the goal (E195's answer).
// Mirror: clang/test/Contracts/p4405-premise-unproven-O0.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }

#include <contracts>
using namespace std::contracts::labels;

void f (int x) pre<diagnose> (x > 3) pre<diagnose> (x > 2) pre<diagnose> (x > 1);

void g1 (int y)
{
  f (y);
}
// { dg-regexp {[^\n]*p4405-premise-unproven-O0\.C:19:5: warning: contract assertion cannot be proven[^\n]*} }
// { dg-regexp {[^\n]*p4405-premise-unproven-O0\.C:19:5: warning: contract assertion cannot be proven[^\n]*} }
// { dg-regexp {[^\n]*p4405-premise-unproven-O0\.C:19:5: warning: contract assertion cannot be proven[^\n]*} }
