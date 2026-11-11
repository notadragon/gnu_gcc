// The names nominated by a `using contract_control namespace X;' directive are
// found only within assertion-control expressions (P3400), by qualified lookup
// as by unqualified lookup: `::lab' is not ambiguous and `N::lab' is not
// found.  The in-label half is contract-control-using-qualified.C.
//
// Mirror: clang/test/Contracts/contract-control-qualified-lookup.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

namespace L { int lab = 2; }
using contract_control namespace L;

namespace A { int lab = 1; }
using namespace A;
int z = ::lab;     // OK, finds A::lab only

namespace N { using contract_control namespace A; }
int w = N::lab;    // { dg-error "is not a member of 'N'" }
