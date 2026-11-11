// Watch test for an open bug (GCC-757, stock GCC too; not a contracts bug):
// a dynamic_cast whose operand points past the end of an array points to
// no object, which is undefined ([expr.dynamic.cast]/7), but constant
// evaluation accepts it and yields a null pointer.  When it is fixed, the
// xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-dynamic-cast-past-end-downcast.cpp.
// { dg-do compile { target c++26 } }

struct B { virtual constexpr ~B () {} };
struct D : B {};
constexpr int
t (int x)
{
  B bs[1];
  B *bp = bs + x;
  D *dp = dynamic_cast<D *> (bp);
  return dp == nullptr ? -1 : 0;
}
static_assert (t (1) == -1);	// { dg-error "non-constant condition" "GCC-757" { xfail *-*-* } }
