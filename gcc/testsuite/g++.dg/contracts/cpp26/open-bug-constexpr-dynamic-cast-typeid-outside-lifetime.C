// Watch test for an open bug (GCC-752, stock GCC too; not a contracts bug):
// typeid and dynamic_cast of a polymorphic object outside its lifetime are
// undefined ([basic.life]/7), but constant evaluation does not reject
// them: typeid of a destroyed automatic object and dynamic_cast of a
// destroyed allocated object whose result is unused are accepted, and when
// the dynamic_cast's result is used it evaluates to a null pointer.  When
// it is fixed, the xfailed dg-error lines report an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-dynamic-cast-typeid-outside-lifetime.cpp.
// { dg-do compile { target c++26 } }

#include <memory>
#include <typeinfo>

struct B { constexpr virtual ~B () {} };
struct D : B {};

constexpr int
t1 ()
{
  D d;
  B &br = d;
  d.~D ();
  (void) typeid (br);		// d is outside its lifetime
  std::construct_at (&d);
  return 1;
}
static_assert (t1 () == 1);	// { dg-error "non-constant condition" "GCC-752" { xfail *-*-* } }

constexpr int
t2 ()
{
  std::allocator<D> a;
  D *p = a.allocate (1);
  B *bp = std::construct_at (p);
  std::destroy_at (p);
  (void) dynamic_cast<D *> (bp);	// *p is outside its lifetime
  a.deallocate (p, 1);
  return 1;
}
static_assert (t2 () == 1);	// { dg-error "non-constant condition" "GCC-752" { xfail *-*-* } }

// The result used: GCC evaluates the cast to a null pointer, so t3 ()
// is 0 and this assertion holds; it should not be a constant expression.
constexpr int
t3 ()
{
  std::allocator<D> a;
  D *p = a.allocate (1);
  B *bp = std::construct_at (p);
  std::destroy_at (p);
  bool r = dynamic_cast<D *> (bp) != nullptr;	// *p is outside its lifetime
  a.deallocate (p, 1);
  return r;
}
static_assert (t3 () == 0);	// { dg-error "non-constant condition" "GCC-752" { xfail *-*-* } }
