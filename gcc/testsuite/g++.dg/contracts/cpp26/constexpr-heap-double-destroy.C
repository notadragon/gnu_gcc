// A new-expression's object ends its lifetime when it is destroyed, although
// its storage stays allocated until it is deallocated.  Destroying it again,
// or using it, is not a core constant expression; constant evaluation used to
// accept all of these for allocated objects (GCC-70, upstream PR126425),
// while diagnosing them for declared ones.  Constructing a new object in the
// storage starts a lifetime again.
//
// Mirror: clang/test/Contracts/constexpr-double-destroy.cpp.
// { dg-do compile { target c++20 } }

#include <memory>

// The two destroying cases are diagnosed where the second destruction ends
// the object's lifetime: at the end of the destructor.  One directive covers
// both, since DejaGnu prunes every matching message on its line.
struct S { float f = 0; constexpr ~S () {} };
// { dg-error "destroying an allocated object outside its lifetime" "" { target *-*-* } .-1 }

constexpr bool
destroy_then_delete ()
{
  S *p = new S;
  p->~S ();
  delete p;
  return true;
}
static_assert (destroy_then_delete ()); // { dg-error "non-constant condition" }

constexpr bool
destroy_twice ()
{
  S *p = new S;
  p->~S ();
  p->~S ();
  std::construct_at (p);
  delete p;
  return true;
}
static_assert (destroy_twice ()); // { dg-error "non-constant condition" }

constexpr bool
write_after_destroy ()
{
  S *p = new S;
  p->~S ();
  p->f = 1; // { dg-error "modification of an allocated object outside its lifetime" }
  std::construct_at (p);
  delete p;
  return true;
}
static_assert (write_after_destroy ()); // { dg-error "non-constant condition" }

constexpr bool
read_after_destroy ()
{
  S *p = new S;
  p->~S ();
  float g = p->f; // { dg-error "accessing an allocated object outside its lifetime" }
  std::construct_at (p);
  delete p;
  return g == 0;
}
static_assert (read_after_destroy ()); // { dg-error "non-constant condition" }

// Destroying, reconstructing and deleting is fine, as is deallocating
// destroyed storage directly.
constexpr bool
reconstruct ()
{
  S *p = new S;
  p->~S ();
  std::construct_at (p);
  p->f = 2;
  float g = p->f;
  delete p;
  return g == 2;
}
static_assert (reconstruct ());

constexpr bool
destroy_then_deallocate ()
{
  std::allocator<S> a;
  S *p = a.allocate (1);
  std::construct_at (p);
  std::destroy_at (p);
  a.deallocate (p, 1);
  return true;
}
static_assert (destroy_then_deallocate ());

// An allocator's storage for several objects: each has its own lifetime, so
// destroying the first leaves the second alive.
constexpr bool
several_objects ()
{
  std::allocator<S> a;
  S *p = a.allocate (2);
  std::construct_at (p);
  std::construct_at (p + 1);
  std::destroy_at (p);
  p[1].f = 3;
  float g = p[1].f;
  std::destroy_at (p + 1);
  a.deallocate (p, 2);
  return g == 3;
}
static_assert (several_objects ());
