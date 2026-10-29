// GCC-676: a label with a compute_semantic member, in a translation unit
// that has not included <contracts>.  Probing the facet looked up
// std::contracts::evaluation_semantic and, not finding it, injected a class
// of that name, then built a semantic value of that class type: an ICE in
// wide_int_to_tree_1.  Without the enumeration there is no facet to call,
// so the member is not one.  p3400-compute-semantic-header-after.C includes
// <contracts> after the label's use.
//
// Mirror: clang/test/Contracts/p3400-compute-semantic-no-header.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

struct L
{
  using assertion_control_object = L;
  constexpr int compute_semantic (int s) const { return s; }
};

constexpr L l;

void f (int x) pre<l> (x > 0) {}
