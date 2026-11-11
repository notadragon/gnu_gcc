// Without -fcontracts-p3400 an assertion-control label is diagnosed and its
// <...> skipped.  When the '>' is missing the skip stops at the end of the
// declaration, so the declarations after it are still parsed and diagnosed;
// it used to run to the end of the file (GCC-665).  The same in a class,
// where the contract is parsed later, and a well-formed label on in-class
// member declarations and definitions reported once each.  With the flag
// the same declaration is contract-label-malformed-recovery.C.
//
// Mirror: clang/test/Contracts/label-recovery-unterminated.cpp in the
// llvm_llvm-project fork (CLANG-648; the class cases are GCC-668).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void j (int x) pre<1 x; // { dg-error "assertion-control labels require" }
// { dg-error "expected" "" { target *-*-* } .-1 }
struct A { int a; };
int after = undeclared; // { dg-error "'undeclared' was not declared" }

struct B {
  void k (int x) pre<1 x; // { dg-error "assertion-control labels require" }
  // { dg-error "expected" "" { target *-*-* } .-1 }
  int b;
};
int after2 = undeclared2; // { dg-error "'undeclared2' was not declared" }

struct L { using assertion_control_object = L; };
constexpr L l {};
struct C {
  void m (int x) pre<l> (x > 0); // { dg-error "assertion-control labels require" }
  void n (int x) pre<l> (x > 0) {} // { dg-error "assertion-control labels require" }
};
