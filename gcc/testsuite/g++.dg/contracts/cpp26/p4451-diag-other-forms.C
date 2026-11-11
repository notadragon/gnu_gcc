// P4451: without -fcontracts-p3400, an assertion-control-specifier other
// than `<terminating>' is rejected on a lambda's contract and on a member
// function declaration too (p4451-diag-other.C), while `<terminating>' on a
// member is accepted.  The declaration <contracts> provides is not a P3400
// assertion-control object: its type has no nested assertion_control_object
// type.
//
// Mirror: clang/test/Contracts/p4451-diag-other.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

auto l = [] (int x) pre<other> (x > 0) { return x; };	// { dg-error "only .<terminating>. is allowed as an assertion-control-specifier without .-fcontracts-p3400." }
struct S
{
  void m (int x) pre<other> (x > 0);	// { dg-error "only .<terminating>. is allowed" }
  void n (int x) pre<terminating> (x > 0);
  void o (int x) { contract_assert<other> (x > 0); }	// { dg-error "only .<terminating>. is allowed" }
};
auto l2 = [] (int x) pre<terminating> (x > 0) { return x; };

template <class T>
concept control = requires { typename T::assertion_control_object; };
using term_t = decltype (std::contracts::labels::terminating);
static_assert (!control<__remove_cvref (term_t)>);
