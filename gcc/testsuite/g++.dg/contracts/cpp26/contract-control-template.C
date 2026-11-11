// contract_control (E) in a template: a dependent operand is made into its
// constexpr object at instantiation, one object for each type and value, and
// it is an lvalue of its deduced type there as outside templates.
//
// Mirror: clang/test/Contracts/contract-control-template.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <type_traits>

struct L { using assertion_control_object = L; int v; };
constexpr L make (int v) { return L{v}; }

template <int N>
constexpr const L *addr () { return &contract_control (make (N)); }
template <class T>
constexpr bool same ()
{
  return std::is_same_v<decltype ((contract_control (T{}))), const T &>;
}

static_assert (addr<3> ()->v == 3);
static_assert (addr<3> () == addr<3> ());
static_assert (same<int> () && same<L> ());

namespace lab { constexpr L l{1}; }
template <class T> T f (T x) pre<contract_control (lab::l)> (x > 0) { return x; }

int main () { return f (1) - 1; }
