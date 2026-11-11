// P4451: without -fcontracts-p4451 (or -fcontracts-p3400), `<terminating>'
// on a lambda's contract and on a member function declaration is pointed at
// the option, as on a namespace-scope function (p4451-diag-no-flag.C).
//
// Mirror: clang/test/Contracts/p4451-diag-no-flag.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

auto l = [] (int x) pre<terminating> (x > 0) { return x; };	// { dg-error "assertion-control-specifier .<terminating>. requires .-fcontracts-p4451." }
struct S
{
  void m (int x) pre<terminating> (x > 0);	// { dg-error "assertion-control-specifier .<terminating>. requires .-fcontracts-p4451." }
  void n (int x) { contract_assert<terminating> (x > 0); }	// { dg-error "assertion-control-specifier .<terminating>. requires .-fcontracts-p4451." }
};
