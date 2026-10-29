// Watch test for an open bug (GCC-831, stock GCC too; not a contracts bug):
// `a[1]' on a one-element array is an indirection through a pointer that
// points to no object, which is undefined ([expr.unary.op]/1), but constant
// evaluation accepts an empty class stored to it, `a[1] = e': the elided
// trivial assignment never evaluates its target.  So is one assigned from
// it, `e = a[1]', and an empty base of a non-empty element stored to or
// assigned from (`static_cast<EB &> (da[1]) = b', `b = da[1]'): the elided
// assignment evaluates its operands as glvalues only.  When it is fixed,
// the xfailed dg-error lines report an XPASS.  Clang rejects the stores and
// the base assigned from; it accepts `e = a[1]' (llvm_llvm-project
// CLANG-779).
// { dg-do compile { target c++26 } }

struct E {};
constexpr bool
f ()
{
  E a[1];
  E e;
  a[1] = e;
  return true;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-831" { xfail *-*-* } }

constexpr bool
from ()
{
  E a[1];
  E e;
  e = a[1];
  return true;
}
static_assert (from ());	// { dg-error "non-constant condition" "GCC-831" { xfail *-*-* } }

struct EB {};
struct D : EB { int x; };
constexpr bool
bstore ()
{
  D da[1]{};
  EB b;
  static_cast<EB &> (da[1]) = b;
  return true;
}
static_assert (bstore ());	// { dg-error "non-constant condition" "GCC-831" { xfail *-*-* } }

constexpr bool
bfrom ()
{
  D da[1]{};
  EB b;
  b = da[1];
  return true;
}
static_assert (bfrom ());	// { dg-error "non-constant condition" "GCC-831" { xfail *-*-* } }

// Control, which passes: the copy from it is rejected.
constexpr bool g () { E a[1]; E e2 = a[1]; (void) e2; return true; }
static_assert (g ());	// { dg-error "non-constant condition" }
// { dg-error "outside the bounds" "" { target *-*-* } .-2 }
