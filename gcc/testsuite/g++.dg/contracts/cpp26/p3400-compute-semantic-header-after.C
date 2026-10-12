// GCC-676: as p3400-compute-semantic-no-header.C, with <contracts>
// included after the label's use.  The enumeration
// std::contracts::evaluation_semantic is declared there and must not
// conflict with anything the label's use left behind.
//
// Mirror: owed to Clang and EDG, recorded in their open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

struct L
{
  using assertion_control_object = L;
  constexpr int compute_semantic (int s) const { return s; }
};

constexpr L l;

void f (int x) pre<l> (x > 0) {}

#include <contracts>

void g (int x) pre (x > 0) {}
