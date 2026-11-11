// Watch test for an open bug (GCC-779, stock GCC 15.3..trunk too; not a
// contracts bug): a placement new at a null pointer is undefined ([expr.new]:
// the non-allocating allocation function returns null), but constant
// evaluation accepts the default-initialization of an empty class there,
// which stores nothing.  When it is fixed, the xfailed dg-error line reports
// an XPASS.  Configured, GCC-728's check (ub:expr.new.non.allocating.null)
// rejects it (p3100-placement-null-constexpr.C).
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/p3100-placement-null-constexpr.cpp, where
// Clang rejects it either way.
// { dg-do compile { target c++26 } }

#include <new>
struct E {};
constexpr bool
f ()
{
  E *p = nullptr;
  new (p) E;
  return true;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-779" { xfail *-*-* } }

// Control, which passes: value-initialization at the null pointer is rejected.
constexpr bool g () { E *p = nullptr; new (p) E (); return true; }	// { dg-error "modification of" }
static_assert (g ());	// { dg-error "non-constant condition" }
