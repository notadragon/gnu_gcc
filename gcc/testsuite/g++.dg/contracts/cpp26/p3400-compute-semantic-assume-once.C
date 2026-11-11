// With the configured semantic assume and no -fcontracts-allow-assume, a
// label whose compute_semantic returns assume (not allowed) is diagnosed
// once for one assertion, though the runtime and the constant-evaluation
// semantics are both resolved (GCC-645).  -fmax-errors=1 turns a second
// copy into "compilation terminated", an excess error.
//
// Mirror: clang/test/Contracts/p3100-label-assume-constexpr-no-flag.cpp in
// the llvm_llvm-project fork, whose -verify counts it once.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=assume -fmax-errors=1" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct compute_assume_t {
  using assertion_control_object = compute_assume_t;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::assume; }
};

int d (int x) pre<compute_assume_t{}> (x > 0) { return x; }  // { dg-error "compute_semantic. result is not in the allowed" }
