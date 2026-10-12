// Watch test for an open bug (GCC-644, stock GCC trunk too; not a contracts
// bug): typeid of a glvalue of polymorphic class type evaluates its operand
// ([expr.typeid]/4 makes only the other case unevaluated), so naming a local
// variable or parameter there odr-uses it, and a lambda with no
// capture-default must capture it explicitly ([expr.prim.lambda.capture]/7
// captures regardless of typeid).  GCC 16 rejects; trunk accepts since the
// two-pass typeid parse (r17-2200, PR125886), which treats the operand as
// unevaluated when the dynamic type is known.  A non-polymorphic operand is
// unevaluated, so naming it there is fine.  When fixed, the xfailed dg-error
// lines report an XPASS.
//
// It bears on contracts through EDG-58's shape: typeid of a by-value
// polymorphic parameter in a postcondition odr-uses it, so it must be const
// (postcondition-params-typeid.C).
//
// Mirror: clang/test/Contracts/typeid-polymorphic-capture.cpp in the
// llvm_llvm-project fork, which passes.
// { dg-do compile { target c++20 } }

#include <typeinfo>

struct B { virtual ~B (); };
struct N { };

void f (B x)
{
  auto l = [] { return &typeid (x); };  // { dg-error "not captured" "GCC-644" { xfail *-*-* } }
}

void g ()
{
  B y;
  auto l = [] { return &typeid (y); };  // { dg-error "not captured" "GCC-644" { xfail *-*-* } }
}

void h (N z, B &r)
{
  auto l = [] { return &typeid (z); };  // OK, unevaluated
  auto m = [&] { return &typeid (r); };  // OK, captured
}
