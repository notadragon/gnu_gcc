// P3100: [[assume]] with quick_enforce -- a false side-effect-free predicate
// fails fast (traps/terminates).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-attr-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "assume quick_enforce" }

__attribute__((noinline)) int f (int x) { [[assume (x > 0)]]; return x; }

int main () { return f (-1); }
