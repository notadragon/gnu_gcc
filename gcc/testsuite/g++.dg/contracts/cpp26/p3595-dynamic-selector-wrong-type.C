// P3595 / DECISIONS.md E37 (a), GCC-672: a declaration the translation
// unit makes with the qualified name of a "C++"-linkage dynamic selector in
// the configuration, but not the selector's type
// std::contracts::evaluation_semantic (), is warned about at that
// declaration (-Wcontract-configuration), whether or not a contract in the
// translation unit uses the selector.  A function of that name with
// parameters, a template, or a "C"-linkage selector name is not.
// p3595-dynamic-selector-wrong-type-quiet.C is the -Wno-contract-configuration
// run.
//
// Mirror: clang/test/Contracts/p3595-dynamic-selector-wrong-type.cpp in the
// llvm_llvm-project fork (CLANG-655).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-selector-wrong-type.json" }

namespace std::contracts {
  enum class evaluation_semantic : unsigned short {
    ignore = 1, observe, enforce, quick_enforce
  };
}

namespace defined {
  int sel () { return 3; } // { dg-warning "function 'int defined::sel\\(\\)' has the name of the dynamic contract selector 'defined::sel' in the contract configuration, but its type 'int\\(\\)' is not 'std::contracts::evaluation_semantic \\(\\)'" }
}

namespace declared {
  long sel (); // { dg-warning "function 'long int declared::sel\\(\\)' has the name of the dynamic contract selector 'declared::sel'" }
}

namespace var {
  std::contracts::evaluation_semantic sel; // { dg-warning "variable 'var::sel' has the name of the dynamic contract selector 'var::sel' in the contract configuration, but it is not a function of type 'std::contracts::evaluation_semantic \\(\\)'" }
}

namespace right {
  std::contracts::evaluation_semantic sel ();
}

namespace other {
  int sel (int);
  template <class T> int sel ();
}

namespace outer {
  inline namespace v1 {
    unsigned short sel (); // { dg-warning "dynamic contract selector 'outer::sel'" }
  }
}

int global_sel (); // { dg-warning "dynamic contract selector 'global_sel'" }

extern "C" int c_sel ();

namespace redecl {
  int sel ();
  int sel () { return 1; } // { dg-warning "dynamic contract selector 'redecl::sel'" }
}

namespace u1 { void f (int x) pre (x > 0) {} }
namespace u2 { void f (int x) pre (x > 0) {} }
namespace u4 { void f (int x) pre (x > 0) {} }
