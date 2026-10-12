// An explicit -fno-contracts-pNNNN wins over the -fcontracts-p3850 umbrella:
// P3400 is off.  The other flag order behaves the same.
//
// Mirror: clang/test/Contracts/umbrella-fno-subflag.cpp and
// umbrella-fno-subflag-driver.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fno-contracts-p3400" }

struct L { using assertion_control_object = L; };
constexpr L lbl{};
int f (int x) pre<lbl> (x > 0) { return x; } // { dg-error "assertion-control labels require" }

#if defined(__cpp_contracts_labels)
#error "__cpp_contracts_labels defined with -fno-contracts-p3400"
#endif
